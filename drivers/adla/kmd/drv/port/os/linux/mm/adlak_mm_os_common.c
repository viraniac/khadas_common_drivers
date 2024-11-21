/*******************************************************************************
 * Copyright (C) 2022 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_mm_os_common.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2022/07/15	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "adlak_mm_os_common.h"

#include "adlak_mm_common.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

void adlak_cma_deinit(struct device *dev) {
    AML_LOG_DEBUG("%s", __func__);
    of_reserved_mem_device_release(dev);
}

int adlak_cma_init(struct device *dev) {
    int ret;
    AML_LOG_DEBUG("%s", __func__);

    /* Initialize CMA */
    ret = of_reserved_mem_device_init(dev);

    if (ret) {
        AML_LOG_WARN("cma mem not present or init failed\n");
        return -1;
    } else {
        AML_LOG_INFO("cma memory init ok\n");
    }
    return 0;
}

#if (CONFIG_ADLAK_MEM_POOL_EN && defined(CONFIG_ADLAK_USE_RESERVED_MEMORY))

void adlak_unmap_region_nocache(struct adlak_mem_pool_info **ptr) {
    struct adlak_mem_pool_info *mem_pool = *ptr;
    if (ADLAK_IS_ERR_OR_NULL(mem_pool)) {
        return;
    }
    if (mem_pool->cpu_addr_base) {
        memunmap(mem_pool->cpu_addr_base);
    }
    adlak_os_free(mem_pool);
    *ptr = NULL;
}

inline int adlak_remap_region_nocache(struct adlak_device *        padlak,
                                      struct adlak_mem_pool_info **ptr) {
    phys_addr_t                 physical;
    size_t                      size;
    void *                      vaddr    = NULL;
    struct adlak_mem_pool_info *mem_pool = *ptr;
#ifdef CONFIG_OF
#error "No support reserved-memory currently when the device-tree enabled."
#endif

    physical = padlak->hw_res.adlak_resmem_pa;
    size     = padlak->hw_res.adlak_resmem_size;
    if (0 == size) {
        goto err;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0)
    vaddr = memremap(physical, size, MEMREMAP_WC);  // write combine
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
    vaddr = memremap(physical, size, MEMREMAP_WT);
#else
    vaddr = ioremap_nocache(physical, size);
#endif

    if (!vaddr) {
        AML_LOG_ERR("fail to map physical to kernel space\n");
        vaddr = NULL;
        goto err;
    }

    mem_pool = adlak_os_zalloc(sizeof(struct adlak_mem_pool_info), ADLAK_GFP_KERNEL);
    if (ADLAK_IS_ERR_OR_NULL(mem_pool)) {
        goto err_alloc;
    }

    mem_pool->cpu_addr_base  = vaddr;
    mem_pool->phys_addr_base = physical;
    mem_pool->dma_addr_base  = physical;
    mem_pool->size           = size;
    mem_pool->mem_src        = ADLAK_ENUM_MEMSRC_RESERVED;
    mem_pool->cacheable      = false;
    *ptr                     = mem_pool;
    AML_LOG_INFO("Reserved memory info: dma_addr= 0x%lX,  phys_addr= 0x%lX,size=%lu MByte\n",
                 (uintptr_t)mem_pool->dma_addr_base, (uintptr_t)mem_pool->phys_addr_base,
                 (uintptr_t)(mem_pool->size / (1024 * 1024)));

    return 0;
err_alloc:

err:
    return -1;
}

#endif
static void adlak_os_free_pages(struct page *pages[], int nr_pages) {
    int i;
    AML_LOG_DEBUG("%s", __func__);
    for (i = 0; i < nr_pages; ++i) {
        if (pages[i]) __free_page(pages[i]);
    }
}
static void adlak_os_free_pages_contiguous(struct page *pages[], int nr_pages) {
    AML_LOG_DEBUG("%s", __func__);
    if (pages[0]) {
        __free_pages(pages[0], get_order(ADLAK_PAGE_ALIGN(nr_pages * ADLAK_PAGE_SIZE)));
    }
}

static int adlak_dma_map_of_discontiguous(struct adlak_mem *mm, struct adlak_mem_handle *mm_info,
                                          uint32_t offset, uint32_t size) {
    int              ret        = ERR(NONE);
    struct sg_table *sgt        = NULL;
    int32_t          result     = 0;
    uint32_t         flush_size = ADLAK_ALIGN(size, cache_line_size());
    AML_LOG_DEBUG("%s", __func__);

    switch (mm_info->req.mem_direction) {
        case ADLAK_ENUM_MEM_DIR_WRITE_ONLY:
            mm_info->direction = DMA_TO_DEVICE;
            break;
        case ADLAK_ENUM_MEM_DIR_READ_ONLY:
            mm_info->direction = DMA_FROM_DEVICE;
            break;
        default:
            mm_info->direction = DMA_BIDIRECTIONAL;
            break;
    }

    if (size & (cache_line_size() - 1)) {
        AML_LOG_ERR("flush cache warning memory size 0x%x not align to cache line size 0x%x", size,
                    cache_line_size());
    }

    mm_info->sgt = adlak_os_malloc(sizeof(struct sg_table), ADLAK_GFP_KERNEL);
    if (!mm_info->sgt) {
        AML_LOG_ERR("failed to alloca memory for flush cache handle");
        ret = -1;
        goto err;
    }

    sgt = (struct sg_table *)mm_info->sgt;

    result = sg_alloc_table_from_pages(sgt, (struct page **)mm_info->pages, mm_info->nr_pages,
                                       offset, flush_size, ADLAK_GFP_KERNEL | __GFP_NOWARN);
    if (unlikely(result < 0)) {
        AML_LOG_ERR("sg_alloc_table_from_pages failed");
        ret = -1;
        goto err;
    }

    result = dma_map_sg(mm->dev, sgt->sgl, sgt->nents, mm_info->direction);
    if (unlikely(result != sgt->nents)) {
        AML_LOG_ERR("dma_map_sg failed");
        ret = -1;
        goto err;
    }

err:
    return ret;
}

static int adlak_dma_unmap_of_discontiguous(struct adlak_mem *       mm,
                                            struct adlak_mem_handle *mm_info) {
    struct sg_table *sgt = NULL;
    AML_LOG_DEBUG("%s", __func__);

    sgt = (struct sg_table *)mm_info->sgt;
    if (sgt != NULL) {
        dma_unmap_sg(mm->dev, sgt->sgl, sgt->nents, DMA_FROM_DEVICE);

        sg_free_table(sgt);

        adlak_os_free(sgt);
        mm_info->sgt = NULL;
    }

    return 0;
}

static int adlak_dma_map_of_contiguous(struct adlak_mem *mm, struct adlak_mem_handle *mm_info,
                                       uint32_t offset, uint32_t size, struct page *page_continue) {
    switch (mm_info->req.mem_direction) {
        case ADLAK_ENUM_MEM_DIR_WRITE_ONLY:
            mm_info->direction = DMA_TO_DEVICE;
            break;
        case ADLAK_ENUM_MEM_DIR_READ_ONLY:
            mm_info->direction = DMA_FROM_DEVICE;
            break;
        default:
            mm_info->direction = DMA_BIDIRECTIONAL;
            break;
    }

    mm_info->dma_addr = dma_map_page(mm->dev, page_continue, offset, size, mm_info->direction);

    if (dma_mapping_error(mm->dev, mm_info->dma_addr)) {
        AML_LOG_ERR("failed to dma map pa 0x%lX\n", (uintptr_t)mm_info->phys_addrs[0]);
        return ERR(EFAULT);
    }
    return ERR(NONE);
}

static void adlak_dma_unmap_of_contiguous(struct adlak_mem *mm, struct adlak_mem_handle *mm_info) {
    if (mm_info->dma_addr) {
        dma_unmap_page(mm->dev, mm_info->dma_addr, mm_info->req.bytes, mm_info->direction);
        mm_info->dma_addr = 0;
    }
}

void adlak_os_free_discontiguous(struct adlak_mem *mm, struct adlak_mem_handle *mm_info) {
    AML_LOG_DEBUG("%s", __func__);

    if (mm_info->pages) {
        adlak_os_mm_vunmap(mm_info);
        adlak_dma_unmap_of_discontiguous(mm, mm_info);
        adlak_os_free_pages((struct page **)mm_info->pages, mm_info->nr_pages);
        adlak_os_free(mm_info->phys_addrs);
        adlak_os_free(mm_info->pages);
        mm_info->pages = NULL;
    }
}

int adlak_os_alloc_discontiguous(struct adlak_mem *mm, struct adlak_mem_handle *mm_info) {
    struct page **pages      = NULL;
    phys_addr_t * phys_addrs = NULL;
    int           i;
    size_t        size = mm_info->req.bytes;
    gfp_t         gfp  = 0;
    AML_LOG_DEBUG("%s", __func__);

    pages = (struct page **)adlak_os_zalloc(sizeof(struct page *) * mm_info->nr_pages,
                                            ADLAK_GFP_KERNEL);
    if (!pages) {
        goto err_alloc_pages;
    }
    mm_info->pages = (void *)pages;

    phys_addrs =
        (phys_addr_t *)adlak_os_zalloc(sizeof(phys_addr_t) * mm_info->nr_pages, ADLAK_GFP_KERNEL);
    if (unlikely(!phys_addrs)) {
        goto err_alloc_phys_addrs;
    }
    mm_info->phys_addrs = phys_addrs;

    gfp |= (GFP_DMA | GFP_USER | __GFP_ZERO);
    if (ADLAK_ENUM_MEMTYPE_INNER_PA_WITHIN_4G & mm_info->req.mem_type) {
        gfp |= (__GFP_DMA32);
    }

    for (i = 0; i < mm_info->nr_pages; ++i) {
        pages[i] = alloc_page(gfp);
        if (unlikely(!pages[i])) {
            goto err_alloc_page;
        }
        phys_addrs[i] = page_to_phys(pages[i]);  // get physical addr
    }

    if (adlak_dma_map_of_discontiguous(mm, mm_info, 0, size)) {
        goto err_flush_init;
    }

    mm_info->phys_addr = -1;  // the phys is not contiguous

    return ERR(NONE);

err_flush_init:
err_alloc_page:
    adlak_os_free(phys_addrs);
    adlak_os_free_pages(pages, i);
err_alloc_phys_addrs:
    adlak_os_free(pages);
err_alloc_pages:

    return ERR(ENOMEM);
}

void adlak_os_free_contiguous(struct adlak_mem *mm, struct adlak_mem_handle *mm_info) {
    AML_LOG_DEBUG("%s", __func__);

    if (mm_info->pages) {
        adlak_os_mm_vunmap(mm_info);
        adlak_dma_unmap_of_contiguous(mm, mm_info);
        adlak_os_free_pages_contiguous((struct page **)mm_info->pages, mm_info->nr_pages);
        adlak_os_free(mm_info->phys_addrs);
        adlak_os_free(mm_info->pages);
        mm_info->pages = NULL;
    }
}

int adlak_os_alloc_contiguous(struct adlak_mem *mm, struct adlak_mem_handle *mm_info) {
    struct page **pages         = NULL;
    struct page * page_continue = NULL;
    phys_addr_t * phys_addrs    = NULL;
    int           i, order;
    size_t        size = mm_info->req.bytes;
    gfp_t         gfp  = 0;

    AML_LOG_DEBUG("%s", __func__);

    pages = (struct page **)adlak_os_zalloc(sizeof(struct page *) * mm_info->nr_pages,
                                            ADLAK_GFP_KERNEL);
    if (!pages) {
        goto err_alloc_pages;
    }
    mm_info->pages = (void *)pages;

    phys_addrs =
        (phys_addr_t *)adlak_os_zalloc(sizeof(phys_addr_t) * mm_info->nr_pages, ADLAK_GFP_KERNEL);
    if (!phys_addrs) {
        goto err_alloc_phys_addrs;
    }
    mm_info->phys_addrs = phys_addrs;

    order = get_order(ADLAK_PAGE_ALIGN(size));
    if (order >= MAX_ORDER) {
        AML_LOG_WARN("contiguous alloc contiguous memory order is bigger than MAX, %d >= %d\n",
                     order, MAX_ORDER);
        goto err_order;
    }
    gfp |= (GFP_DMA | GFP_USER | __GFP_ZERO);
    if (ADLAK_ENUM_MEMTYPE_INNER_PA_WITHIN_4G & mm_info->req.mem_type) {
        gfp |= (__GFP_DMA32);
    }
    page_continue = alloc_pages(gfp, order);
    if (unlikely(!page_continue)) {
        AML_LOG_ERR("alloc_pages %d fail", order);
        goto err_alloc_page;
    }

    AML_LOG_DEBUG("page_continue=0x%lX", (uintptr_t)page_continue);
    phys_addrs[0] = page_to_phys(page_continue);
    for (i = 0; i < mm_info->nr_pages; ++i) {
        phys_addrs[i] = phys_addrs[0] + i * ADLAK_PAGE_SIZE;  // get physical addr
        pages[i]      = phys_to_page(phys_addrs[i]);
    }

    mm_info->phys_addr = phys_addrs[0];

    if (adlak_dma_map_of_contiguous(mm, mm_info, 0, size, page_continue)) {
        goto err_flush_init;
    }

    return ERR(NONE);

err_flush_init:
    adlak_os_free_pages_contiguous(pages, mm_info->nr_pages);

err_alloc_page:
err_order:
    adlak_os_free(phys_addrs);

err_alloc_phys_addrs:
    adlak_os_free(pages);
err_alloc_pages:

    return ERR(ENOMEM);
}

int adlak_os_attach_ext_mem(struct adlak_mem *mm, struct adlak_mem_handle *mm_info,
                            uint64_t phys_addr) {
    phys_addr_t *phys_addrs    = NULL;
    void *       cpu_addr      = NULL;
    struct page *page_continue = NULL;
    int          i;
    AML_LOG_DEBUG("%s", __func__);
    AML_LOG_DEBUG("phys_addr:0x%lX, size:0x%lX", (uintptr_t)phys_addr,
                  (uintptr_t)mm_info->req.bytes);

    mm_info->phys_addrs = NULL;
    phys_addrs =
        (phys_addr_t *)adlak_os_zalloc(sizeof(phys_addr_t) * mm_info->nr_pages, ADLAK_GFP_KERNEL);
    if (!phys_addrs) {
        goto err_alloc_phys_addrs;
    }

    mm_info->phys_addr = phys_addr;
    for (i = 0; i < mm_info->nr_pages; ++i) {
        phys_addrs[i] = phys_addr + (i * ADLAK_PAGE_SIZE);  // get physical addr
        AML_LOG_DEBUG("phys_addrs[%d]=0x%llx", i, (uint64_t)(uintptr_t)phys_addrs[i]);
    }
    mm_info->pages      = NULL;
    mm_info->phys_addrs = phys_addrs;
    mm_info->cpu_addr   = cpu_addr;
    mm_info->dma_addr   = (dma_addr_t)NULL;

    page_continue = phys_to_page(mm_info->phys_addr);
    if (adlak_dma_map_of_contiguous(mm, mm_info, 0, mm_info->req.bytes, page_continue)) {
        adlak_os_free(phys_addrs);
    }

    return ERR(NONE);
err_alloc_phys_addrs:

    return ERR(ENOMEM);
}

void adlak_os_dettach_ext_mem(struct adlak_mem *mm, struct adlak_mem_handle *mm_info) {
    AML_LOG_DEBUG("%s", __func__);
    if (mm_info->phys_addrs) {
        adlak_dma_unmap_of_contiguous(mm, mm_info);
        adlak_os_free(mm_info->phys_addrs);
    }
}

int adlak_os_mmap(struct adlak_mem *mm, struct adlak_mem_handle *mm_info, void *const _vma) {
    unsigned long                pfn;
    int                          i;
    struct vm_area_struct *const vma = (struct vm_area_struct *const)_vma;
    pgprot_t                     vm_page_prot;
    struct page **               pages = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
    vma->vm_flags |= (VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_DONTDUMP);
#else
    vma->vm_flags |= (VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_RESERVED);
#endif

    // always remap as cacheable Virtual Memory Area
    vm_page_prot = vm_get_page_prot(vma->vm_flags); /*cacheable*/
    // set as cacheable in userspace
    mm_info->mem_type = mm_info->mem_type | ADLAK_ENUM_MEMTYPE_INNER_USER_CACHEABLE;
#if 0
    vma->vm_page_prot =    pgprot_writecombine(vma->vm_page_prot);/*uncacheable + reorder*/
#endif

    if (mm_info->mem_src == ADLAK_ENUM_MEMSRC_OS) {
        if (mm_info->mem_type & ADLAK_ENUM_MEMTYPE_INNER_CONTIGUOUS) {
            pfn = page_to_pfn(phys_to_page(mm_info->phys_addr));
            remap_pfn_range(vma, vma->vm_start, pfn, vma->vm_end - vma->vm_start,
                            vma->vm_page_prot);
            AML_LOG_DEBUG("%s contiguous  phys_addr = 0x%lX", __func__,
                          (uintptr_t)mm_info->phys_addr);
        } else {
            AML_LOG_DEBUG("%s discontiguous phys_addr = 0x%lX", __func__,
                          (uintptr_t)mm_info->phys_addr);
            pages = (struct page **)(mm_info->pages);
            for (i = 0; i < mm_info->nr_pages; ++i) {
                pfn = page_to_pfn(pages[i]);
                if (remap_pfn_range(vma, vma->vm_start + (i * ADLAK_PAGE_SIZE), pfn,
                                    ADLAK_PAGE_SIZE, vma->vm_page_prot)) {
                    return ERR(EAGAIN);
                }
            }
        }
    } else if (mm_info->mem_src == ADLAK_ENUM_MEMSRC_RESERVED) {
        pfn = page_to_pfn(phys_to_page(mm_info->phys_addr));
        remap_pfn_range(vma, vma->vm_start, pfn, mm_info->req.bytes, vma->vm_page_prot);

    } else if (mm_info->mem_src == ADLAK_ENUM_MEMSRC_CMA) {
        pfn = page_to_pfn(phys_to_page(dma_to_phys(mm->dev, mm_info->dma_addr)));
        remap_pfn_range(vma, vma->vm_start, pfn, mm_info->req.bytes, vma->vm_page_prot);
    } else if (mm_info->mem_src == ADLAK_ENUM_MEMSRC_EXT_PHYS) {
        pfn = page_to_pfn(phys_to_page(mm_info->phys_addr));
        remap_pfn_range(vma, vma->vm_start, pfn, vma->vm_end - vma->vm_start, vma->vm_page_prot);
        AML_LOG_DEBUG("%s ext phys buffer,  phys_addr = 0x%lX", __func__,
                      (uintptr_t)mm_info->phys_addr);
    } else {
        AML_LOG_ERR("Not support memory src [%d]", mm_info->mem_src);
    }

    return 0;
}

static void adlak_dma_sync_sg_partial(struct device *dev, struct scatterlist *sglist, int nents,
                                      unsigned int offset, size_t nbytes,
                                      enum dma_data_direction direction) {
    int                 i;
    unsigned int        sg_size, seg_offset, len;
    struct scatterlist *sg;

    seg_offset = offset;
    for_each_sg(sglist, sg, nents, i) {
        sg_size = sg_dma_len(sg);
        if (seg_offset >= sg_size) {
            seg_offset = seg_offset - sg_size;
            continue;
        }
        len = min(nbytes, (size_t)(sg_size - seg_offset));
        if (DMA_TO_DEVICE == direction) {
            dma_sync_single_for_device(dev, sg_dma_address(sg) + seg_offset, len, direction);
        } else if (DMA_FROM_DEVICE == direction) {
            dma_sync_single_for_cpu(dev, sg_dma_address(sg) + seg_offset, len, direction);
        } else {
            ASSERT(0);
        }
        seg_offset = 0;
        nbytes -= len;
        if (0 == nbytes) {
            break;
        }
    }
}

void adlak_os_flush_cache(struct adlak_mem *mm, struct adlak_mem_handle *mm_info,
                          struct adlak_sync_cache_ext_info *sync_cache_ext_info) {
    struct sg_table *sgt = NULL;
    AML_LOG_DEBUG("%s", __func__);
    if (!(mm_info->mem_type &
          (ADLAK_ENUM_MEMTYPE_INNER_USER_CACHEABLE | ADLAK_ENUM_MEMTYPE_INNER_KERNEL_CACHEABLE))) {
        return;
    }

    AML_LOG_DEBUG(
        "%s mem_type %lX\tcpu_addr %lX\t"
        "cpu_addr_user %lX \nis_partial %d\toffset %lX\tsize %lX\n",
        __func__, (uintptr_t)mm_info->mem_type, (uintptr_t)mm_info->cpu_addr,
        (uintptr_t)mm_info->cpu_addr_user, (int32_t)sync_cache_ext_info->is_partial,
        (uintptr_t)sync_cache_ext_info->offset, (uintptr_t)sync_cache_ext_info->size);
    if (ADLAK_ENUM_MEMTYPE_INNER_CONTIGUOUS & mm_info->mem_type) {
        ASSERT(mm_info->dma_addr);
        if (0 == sync_cache_ext_info->is_partial) {
            dma_sync_single_for_device(mm->dev, mm_info->dma_addr, mm_info->req.bytes,
                                       DMA_TO_DEVICE);
        } else {
            dma_sync_single_for_device(mm->dev, (mm_info->dma_addr + sync_cache_ext_info->offset),
                                       sync_cache_ext_info->size, DMA_TO_DEVICE);
        }
    } else {
        ASSERT(mm_info->sgt);
        sgt = (struct sg_table *)mm_info->sgt;
        if (0 == sync_cache_ext_info->is_partial) {
            dma_sync_sg_for_device(mm->dev, sgt->sgl, sgt->nents, DMA_TO_DEVICE);
        } else {
            adlak_dma_sync_sg_partial(mm->dev, sgt->sgl, sgt->nents, sync_cache_ext_info->offset,
                                      sync_cache_ext_info->size, DMA_TO_DEVICE);
        }
    }
}

void adlak_os_invalid_cache(struct adlak_mem *mm, struct adlak_mem_handle *mm_info,
                            struct adlak_sync_cache_ext_info *sync_cache_ext_info) {
    struct sg_table *sgt = NULL;
    AML_LOG_DEBUG("%s", __func__);
    if (!(mm_info->mem_type &
          (ADLAK_ENUM_MEMTYPE_INNER_USER_CACHEABLE | ADLAK_ENUM_MEMTYPE_INNER_KERNEL_CACHEABLE))) {
        return;
    }
    AML_LOG_DEBUG(
        "%s mem_type %lX\tcpu_addr %lX\t"
        "cpu_addr_user %lX \nis_partial %d\toffset %lX\tsize %lX\n",
        __func__, (uintptr_t)mm_info->mem_type, (uintptr_t)mm_info->cpu_addr,
        (uintptr_t)mm_info->cpu_addr_user, sync_cache_ext_info->is_partial,
        (uintptr_t)sync_cache_ext_info->offset, (uintptr_t)sync_cache_ext_info->size);
    if (ADLAK_ENUM_MEMTYPE_INNER_CONTIGUOUS & mm_info->mem_type) {
        ASSERT(mm_info->dma_addr);
        if (0 == sync_cache_ext_info->is_partial) {
            dma_sync_single_for_cpu(mm->dev, mm_info->dma_addr, mm_info->req.bytes,
                                    DMA_FROM_DEVICE);
        } else {
            dma_sync_single_for_cpu(mm->dev, (mm_info->dma_addr + sync_cache_ext_info->offset),
                                    sync_cache_ext_info->size, DMA_FROM_DEVICE);
        }
    } else {
        ASSERT(mm_info->sgt);
        sgt = (struct sg_table *)mm_info->sgt;
        if (0 == sync_cache_ext_info->is_partial) {
            dma_sync_sg_for_cpu(mm->dev, sgt->sgl, sgt->nents, DMA_FROM_DEVICE);
        } else {
            adlak_dma_sync_sg_partial(mm->dev, sgt->sgl, sgt->nents, sync_cache_ext_info->offset,
                                      sync_cache_ext_info->size, DMA_FROM_DEVICE);
        }
    }
}

void adlak_free_through_dma(struct adlak_mem *mm, struct adlak_mem_handle *mm_info) {
    if (mm_info->cpu_addr) {
        dma_free_coherent(mm->dev, mm_info->req.bytes, mm_info->cpu_addr, mm_info->dma_addr);
        mm_info->cpu_addr = NULL;
    }
}

int adlak_malloc_through_dma(struct adlak_mem *mm, struct adlak_mem_handle *mm_info) {
    gfp_t gfp = 0;
    gfp |= (GFP_DMA | GFP_USER | __GFP_ZERO);
    if (ADLAK_ENUM_MEMTYPE_INNER_PA_WITHIN_4G & mm_info->req.mem_type) {
        gfp |= (GFP_DMA32);
    }

    mm_info->cpu_addr =
        dma_alloc_coherent(mm->dev, (size_t)mm_info->req.bytes, &mm_info->dma_addr, gfp);
    if (!mm_info->cpu_addr) {
        AML_LOG_ERR("failed to dma_alloc %lu bytes\n", (uintptr_t)mm_info->req.bytes);
        return ERR(ENOMEM);
    }
    mm_info->phys_addr = dma_to_phys(mm->dev, mm_info->dma_addr);
    mm_info->mem_src   = ADLAK_ENUM_MEMSRC_CMA;
    mm_info->mem_type  = ADLAK_ENUM_MEMTYPE_INNER_CONTIGUOUS;  // uncacheable|contiguous
    mm_info->iova_addr = mm_info->phys_addr;
    AML_LOG_DEBUG(
        "%s: dma_addr= 0x%lX,  phys_addr= "
        "0x%lX,size=%lu KByte\n",
        __FUNCTION__, (uintptr_t)mm_info->dma_addr, (uintptr_t)mm_info->phys_addr,
        (uintptr_t)(mm_info->req.bytes / 1024));
    return ERR(NONE);
}

int adlak_os_mmap2userspace(struct adlak_mem *mm, struct adlak_mem_handle *mm_info) {
    int                    ret            = ERR(EINVAL);
    unsigned long          addr_userspace = 0;
    struct vm_area_struct *vma            = NULL;
    AML_LOG_DEBUG("%s", __func__);

    if (ADLAK_IS_ERR_OR_NULL(mm_info)) goto exit;
    addr_userspace = vm_mmap(NULL, 0L, mm_info->req.bytes, PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_NORESERVE, 0);

    AML_LOG_DEBUG("vm_mmap addr = 0x%lX", addr_userspace);
    if (ADLAK_IS_ERR_OR_NULL((void *)addr_userspace)) {
        ret = ERR(ENOMEM);
        AML_LOG_ERR("Unable to mmap process text, errno %d\n", ret);
        addr_userspace = 0;
        goto exit;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    mmap_write_lock(current->mm);
#else
    down_write(&current->mm->mmap_sem);
#endif

    vma = find_vma(current->mm, (unsigned long)addr_userspace);
    if (NULL == vma) {
        AML_LOG_ERR("failed to find vma\n");
        vm_munmap(addr_userspace, mm_info->req.bytes);
        addr_userspace = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
        mmap_write_unlock(current->mm);
#else
        up_write(&current->mm->mmap_sem);
#endif
        goto exit;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    mmap_write_unlock(current->mm);
#else
    up_write(&current->mm->mmap_sem);
#endif

    ret = adlak_os_mmap(mm, mm_info, (void *)vma);

exit:
    mm_info->cpu_addr_user = (void *)addr_userspace;

    AML_LOG_DEBUG("cpu_addr_user = 0x%lX", addr_userspace);

    return ret;
}

void adlak_os_unmmap_userspace(struct adlak_mem *mm, struct adlak_mem_handle *mm_info) {
    unsigned long addr_userspace = 0;
    AML_LOG_DEBUG("%s", __func__);

    if (unlikely(!current->mm)) {
        /* the task is exiting. */
        return;
    }

    AML_LOG_DEBUG("%s cpu_addr_user = 0x%lX", __func__, (uintptr_t)mm_info->cpu_addr_user);
    addr_userspace = (unsigned long)mm_info->cpu_addr_user;
    if (addr_userspace) {
        AML_LOG_DEBUG("unmap user space addr=0x%lX", addr_userspace);
        vm_munmap(addr_userspace, mm_info->req.bytes);
        mm_info->cpu_addr_user = 0;
    }
}

void *adlak_os_mm_vmap(struct adlak_mem_handle *mm_info) {
    struct page **pages    = NULL;
    void *        cpu_addr = NULL;
    if (NULL != mm_info->cpu_addr) {
        return mm_info->cpu_addr;
    }
    pages = mm_info->pages;

    /**make a long duration mapping of multiple physical pages into a contiguous virtual space**/

    if (!(mm_info->req.mem_type & ADLAK_ENUM_MEMTYPE_INNER_KERNEL_CACHEABLE)) {
        cpu_addr = vmap(pages, mm_info->nr_pages, VM_MAP, pgprot_writecombine(PAGE_KERNEL));
    } else {
        cpu_addr          = vmap(pages, mm_info->nr_pages, VM_MAP, PAGE_KERNEL);
        mm_info->mem_type = mm_info->mem_type | ADLAK_ENUM_MEMTYPE_INNER_KERNEL_CACHEABLE;
    }
    if (!cpu_addr) {
        goto err_vmap;
    }
    mm_info->cpu_addr = cpu_addr;

    AML_LOG_DEBUG("%s: VA_kernel=0x%lX", __FUNCTION__, (uintptr_t)cpu_addr);

err_vmap:

    return mm_info->cpu_addr;
}

void adlak_os_mm_vunmap(struct adlak_mem_handle *mm_info) {
    if (NULL != mm_info->cpu_addr) {
        /* ummap kernel space */
        vunmap(mm_info->cpu_addr);
        mm_info->cpu_addr = NULL;
    }
    AML_LOG_DEBUG("%s", __func__);
}
