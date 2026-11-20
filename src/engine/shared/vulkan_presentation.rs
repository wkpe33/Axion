use ash::{khr::swapchain, vk};
use std::ffi::{c_char, c_void, CStr};
use std::fs::File;
use std::io::Read;
use std::path::{Path, PathBuf};
use std::ptr;

// All handles, create-info structures, and function pointers use the Vulkan ABI.
// The render thread owns the context and destroys its swapchains before the device.
pub struct Presentation {
    device: ash::Device,
    pipeline_cache: vk::PipelineCache,
    cache_path: Option<PathBuf>,
    functions: swapchain::DeviceFn,
}

fn valid_pipeline_cache(data: &[u8], vendor_id: u32, device_id: u32, uuid: &[u8]) -> bool {
    let read_u32 = |offset: usize| {
        data.get(offset..offset + 4)
            .and_then(|value| <[u8; 4]>::try_from(value).ok().map(u32::from_le_bytes))
    };
    read_u32(0) == Some(32)
        && read_u32(4) == Some(1)
        && read_u32(8) == Some(vendor_id)
        && read_u32(12) == Some(device_id)
        && data.get(16..32) == Some(uuid)
}

const MAX_CACHE_BYTES: usize = 32 * 1024 * 1024;

fn read_pipeline_cache(path: &Path, vendor: u32, device: u32, uuid: &[u8]) -> Option<Vec<u8>> {
    let mut file = File::open(path).ok()?;
    let mut header = [0; 32];
    file.read_exact(&mut header).ok()?;
    if !valid_pipeline_cache(&header, vendor, device, uuid) {
        return None;
    }

    let size = usize::try_from(file.metadata().ok()?.len()).ok()?;
    if !(header.len()..=MAX_CACHE_BYTES).contains(&size) {
        return None;
    }
    let mut data = vec![0; size];
    data[..header.len()].copy_from_slice(&header);
    file.read_exact(&mut data[header.len()..]).ok()?;
    Some(data)
}

#[no_mangle]
pub extern "C" fn axion_vk_presentation_create(
    device: vk::Device,
    get_proc: vk::PFN_vkGetDeviceProcAddr,
    cache_path: *const c_char,
    vendor_id: u32,
    device_id: u32,
    cache_uuid: *const u8,
) -> *mut Presentation {
    let mut missing_required = false;
    let functions = swapchain::DeviceFn::load(|name| {
        // SAFETY: The caller provides the live device and its Vulkan loader.
        let function = unsafe { get_proc(device, name.as_ptr()) };
        if function.is_none()
            && matches!(
                name.to_bytes(),
                b"vkCreateSwapchainKHR"
                    | b"vkDestroySwapchainKHR"
                    | b"vkGetSwapchainImagesKHR"
                    | b"vkAcquireNextImageKHR"
                    | b"vkQueuePresentKHR"
            )
        {
            missing_required = true;
        }
        function.map_or(ptr::null(), |function| function as *const c_void)
    });
    if missing_required {
        return ptr::null_mut();
    }

    // SAFETY: The caller supplies the Vulkan device's own dispatch table.
    let device = unsafe {
        ash::Device::load_with(
            |name| {
                // This device wrapper is used exclusively for the three core cache calls.
                match name.to_bytes() {
                    b"vkCreatePipelineCache"
                    | b"vkGetPipelineCacheData"
                    | b"vkDestroyPipelineCache" => get_proc(device, name.as_ptr())
                        .map_or(ptr::null(), |function| function as *const c_void),
                    _ => ptr::null(),
                }
            },
            device,
        )
    };
    let cache_path = if cache_path.is_null() {
        None
    } else {
        // SAFETY: The caller supplies a null-terminated UTF-8 cache filename.
        unsafe { CStr::from_ptr(cache_path) }
            .to_str()
            .ok()
            .map(PathBuf::from)
    };
    let cached_data = cache_path.as_deref().and_then(|path| {
        // SAFETY: The caller supplies the physical device's 16-byte UUID.
        let uuid = unsafe { cache_uuid.cast::<[u8; 16]>().as_ref() }?;
        read_pipeline_cache(path, vendor_id, device_id, uuid)
    });
    let create_cache = |data: &[u8]| {
        let info = vk::PipelineCacheCreateInfo::default().initial_data(data);
        unsafe { device.create_pipeline_cache(&info, None) }
    };
    let pipeline_cache = cached_data
        .as_deref()
        .and_then(|data| create_cache(data).ok())
        .or_else(|| create_cache(&[]).ok())
        .unwrap_or_default();

    Box::into_raw(Box::new(Presentation {
        device,
        pipeline_cache,
        cache_path,
        functions,
    }))
}

impl Presentation {
    unsafe fn cache_bytes(&self) -> Option<Vec<u8>> {
        let read = self.device.fp_v1_0().get_pipeline_cache_data;
        let mut size = 0;
        let result = unsafe {
            read(
                self.device.handle(),
                self.pipeline_cache,
                &mut size,
                ptr::null_mut(),
            )
        };
        if result != vk::Result::SUCCESS || !(32..=MAX_CACHE_BYTES).contains(&size) {
            return None;
        }
        let mut data = vec![0u8; size];
        let result = unsafe {
            read(
                self.device.handle(),
                self.pipeline_cache,
                &mut size,
                data.as_mut_ptr().cast(),
            )
        };
        if result != vk::Result::SUCCESS || !(32..=data.len()).contains(&size) {
            return None;
        }
        data.truncate(size);
        Some(data)
    }
}

#[no_mangle]
pub unsafe extern "C" fn axion_vk_presentation_destroy(presentation: *mut Presentation) {
    // SAFETY: The caller releases this allocation once, after the last use.
    let presentation = unsafe { Box::from_raw(presentation) };
    if presentation.pipeline_cache != vk::PipelineCache::null() {
        if let Some(path) = &presentation.cache_path {
            if let Some(data) = unsafe { presentation.cache_bytes() } {
                // Publish a complete file so an interrupted save preserves the previous cache.
                let temporary = path.with_extension(format!("{}.tmp", std::process::id()));
                if std::fs::write(&temporary, data).is_ok() {
                    let _ = std::fs::rename(&temporary, path);
                }
                let _ = std::fs::remove_file(temporary);
            }
        }

        unsafe {
            presentation
                .device
                .destroy_pipeline_cache(presentation.pipeline_cache, None)
        };
    }
}

#[no_mangle]
pub extern "C" fn axion_vk_pipeline_cache(presentation: &Presentation) -> vk::PipelineCache {
    presentation.pipeline_cache
}

#[no_mangle]
pub unsafe extern "C" fn axion_vk_swapchain_create(
    presentation: &Presentation,
    info: *const vk::SwapchainCreateInfoKHR<'_>,
    swapchain: *mut vk::SwapchainKHR,
) -> vk::Result {
    unsafe {
        (presentation.functions.create_swapchain_khr)(
            presentation.device.handle(),
            info,
            ptr::null(),
            swapchain,
        )
    }
}

#[no_mangle]
pub unsafe extern "C" fn axion_vk_swapchain_destroy(
    presentation: &Presentation,
    swapchain: vk::SwapchainKHR,
) {
    unsafe {
        (presentation.functions.destroy_swapchain_khr)(
            presentation.device.handle(),
            swapchain,
            ptr::null(),
        )
    }
}

#[no_mangle]
pub unsafe extern "C" fn axion_vk_swapchain_images(
    presentation: &Presentation,
    swapchain: vk::SwapchainKHR,
    count: *mut u32,
    images: *mut vk::Image,
) -> vk::Result {
    unsafe {
        (presentation.functions.get_swapchain_images_khr)(
            presentation.device.handle(),
            swapchain,
            count,
            images,
        )
    }
}

#[no_mangle]
pub unsafe extern "C" fn axion_vk_acquire_image(
    presentation: &Presentation,
    swapchain: vk::SwapchainKHR,
    semaphore: vk::Semaphore,
    index: *mut u32,
) -> vk::Result {
    unsafe {
        (presentation.functions.acquire_next_image_khr)(
            presentation.device.handle(),
            swapchain,
            u64::MAX,
            semaphore,
            vk::Fence::null(),
            index,
        )
    }
}

#[no_mangle]
pub unsafe extern "C" fn axion_vk_queue_present(
    presentation: &Presentation,
    queue: vk::Queue,
    info: *const vk::PresentInfoKHR<'_>,
) -> vk::Result {
    unsafe { (presentation.functions.queue_present_khr)(queue, info) }
}

#[cfg(test)]
mod tests {
    use super::valid_pipeline_cache;

    #[test]
    fn rejects_truncated_or_incompatible_cache() {
        let mut data = Vec::new();
        for field in [32u32, 1, 0x10de, 123] {
            data.extend_from_slice(&field.to_le_bytes());
        }
        data.extend_from_slice(&[7; 16]);
        assert!(valid_pipeline_cache(&data, 0x10de, 123, &[7; 16]));
        assert!(!valid_pipeline_cache(&data[..31], 0x10de, 123, &[7; 16]));
        assert!(!valid_pipeline_cache(&data, 0x8086, 123, &[7; 16]));
        assert!(!valid_pipeline_cache(&data, 0x10de, 456, &[7; 16]));
        assert!(!valid_pipeline_cache(&data, 0x10de, 123, &[8; 16]));
        data[4] = 2;
        assert!(!valid_pipeline_cache(&data, 0x10de, 123, &[7; 16]));
    }
}
