// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <drm/drmP.h>
#include <drm/drm_gem.h>
#include <drm/drm_vma_manager.h>
#include <drm/drm_gem_cma_helper.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dma-buf.h>
#include <linux/amlogic/ion.h>
#include <linux/amlogic/meson_uvm_core.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <dev_ion.h>
#include "meson_gem.h"

#include "linux/amlogic/media/dmabuf_heaps/amlogic_dmabuf_heap.h"
#include <linux/dma-heap.h>
#include <linux/dma-direction.h>
#include <uapi/linux/dma-heap.h>

#define to_am_meson_gem_obj(x) container_of(x, struct am_meson_gem_object, base)
#define uvm_to_gem_obj(x) container_of(x, struct am_meson_gem_object, ubo)
#define MESON_GEM_NAME "meson_gem"

#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
static void zap_dma_buf_range(struct page *dma_page, unsigned long len)
{
	struct page *page;
	void *vaddr;
	int num_pages = len / PAGE_SIZE;

	if (PageHighMem(dma_page)) {
		page = dma_page;

		while (num_pages > 0) {
			vaddr = kmap_atomic(page);
			memset(vaddr, 0, PAGE_SIZE);
			kunmap_atomic(vaddr);

			/*
			 *Avoid wasting time zeroing memory if the process
			 *has been killed by SIGKILL
			 */
			if (fatal_signal_pending(current))
				return;

			page++;
			num_pages--;
		}
	} else {
		memset(page_address(dma_page), 0, PAGE_ALIGN(len));
	}
}
#endif

static int am_meson_gem_alloc_ion_buff(struct am_meson_gem_object *
				       meson_gem_obj, int flags)
{
#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
	int i;
	struct dma_heap *heap = NULL;
	struct dma_buf_attachment *attachment = NULL;
	struct sg_table *sg_table = NULL;
	struct page *page;
	bool from_heap_codecmm = false;
	char DMAHEAP[][20] = {"heap-fb", "heap-gfx", "heap-codecmm"};
#endif
#ifdef CONFIG_AMLOGIC_ION_DEV
	size_t len;
	unsigned int id;
	struct ion_buffer *buffer;
#endif
	u32 bscatter = 0;
	struct dma_buf *dmabuf = NULL;

	if (!meson_gem_obj)
		return -EINVAL;

	/*
	 *check flags to set different ion heap type.
	 *if flags is set to 0, need to use ion dma buffer.
	 */
	if (((flags & (MESON_USE_SCANOUT | MESON_USE_CURSOR)) != 0) || flags == 0) {
#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
		for (i = 0; i < 3; i++) {
			heap = dma_heap_find(DMAHEAP[i]);
			if (!IS_ERR_OR_NULL(heap)) {
				dmabuf = dma_heap_buffer_alloc(heap, meson_gem_obj->base.size,
					O_RDWR, DMA_HEAP_VALID_HEAP_FLAGS);
				if (!IS_ERR_OR_NULL(dmabuf)) {
					DRM_DEBUG("%s alloc success.\n", DMAHEAP[i]);
					if (i == 2)
						from_heap_codecmm = true;
					meson_gem_obj->is_dma = true;
					break;
				}
				if (IS_ERR_OR_NULL(dmabuf) && i == 2)
					DRM_ERROR("%s: dma_heap_alloc fail.\n", __func__);
			} else {
				DRM_DEBUG("%s: dma_heap_find %s fail.\n", __func__, DMAHEAP[i]);
				if (i == 2)
					DRM_ERROR("%s: dma_heap_find fail.\n", __func__);
			}
		}
		DRM_DEBUG("%s: dmabuf(%p) dma_heap_alloc success. size = %zu\n",
			__func__, dmabuf, meson_gem_obj->base.size);
#endif
#ifdef CONFIG_AMLOGIC_ION_DEV

		if (!meson_gem_obj->is_dma) {
			id = meson_ion_fb_heap_id_get();
			if (id) {
				dmabuf = ion_alloc(meson_gem_obj->base.size, (1 << id),
					ION_FLAG_EXTEND_MESON_HEAP);
			} else {
				id = meson_ion_cma_heap_id_get();
				if (id) {
					dmabuf = ion_alloc(meson_gem_obj->base.size, (1 << id),
						ION_FLAG_EXTEND_MESON_HEAP);
					if (IS_ERR_OR_NULL(dmabuf)) {
						DRM_ERROR("%s: ion_alloc fail.\n", __func__);
						return -ENOMEM;
					}
				} else {
					DRM_ERROR("%s: ion_heap_id_get fail.\n", __func__);
					return -ENOMEM;
				}
			}
		}
		DRM_DEBUG("%s: dmabuf(%p) ion_alloc success. size = %zu\n",
			__func__, dmabuf, meson_gem_obj->base.size);
#endif
	} else if (flags & MESON_USE_VIDEO_PLANE) {
		meson_gem_obj->is_uvm = true;
#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
		heap = dma_heap_find("heap-codecmm");
		if (!IS_ERR_OR_NULL(heap)) {
			dmabuf = dma_heap_buffer_alloc(heap, meson_gem_obj->base.size, O_RDWR,
				DMA_HEAP_VALID_HEAP_FLAGS);
			meson_gem_obj->is_dma = true;
		}
#endif
#ifdef CONFIG_AMLOGIC_ION_DEV
		if (!meson_gem_obj->is_dma) {
			id = meson_ion_codecmm_heap_id_get();
			if (id)
				dmabuf = ion_alloc(meson_gem_obj->base.size, (1 << id),
							   ION_FLAG_EXTEND_MESON_HEAP);
		}
#endif
	} else if (flags & MESON_USE_VIDEO_AFBC) {
		meson_gem_obj->is_uvm = true;
		meson_gem_obj->is_afbc = true;
#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
		heap = dma_heap_find("system");
		if (!IS_ERR_OR_NULL(heap)) {
			dmabuf = dma_heap_buffer_alloc(heap, UVM_FAKE_SIZE, O_RDWR, 0);
			meson_gem_obj->is_dma = true;
		}
#endif
#ifdef CONFIG_AMLOGIC_ION
		if (!meson_gem_obj->is_dma)
			dmabuf = ion_alloc(UVM_FAKE_SIZE, ION_HEAP_SYSTEM, 0);
#endif
	} else {
#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
		heap = dma_heap_find("system");
		if (!IS_ERR_OR_NULL(heap)) {
			dmabuf = dma_heap_buffer_alloc(heap, meson_gem_obj->base.size, O_RDWR, 0);
			meson_gem_obj->is_dma = true;
		}
#endif
#ifdef CONFIG_AMLOGIC_ION
		if (!meson_gem_obj->is_dma)
			dmabuf = ion_alloc(meson_gem_obj->base.size,
					   ION_HEAP_SYSTEM, 0);
#endif
		bscatter = 1;
	}

	if (IS_ERR_OR_NULL(dmabuf))
		return PTR_ERR(dmabuf);
	meson_gem_obj->dmabuf = dmabuf;

#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
	if (meson_gem_obj->is_dma) {
		attachment = dma_buf_attach(dmabuf, meson_gem_obj->base.dev->dev);
		if (!attachment) {
			DRM_ERROR("%s: dma_buf_attach fail", __func__);
			return -ENOMEM;
		}

		sg_table = dma_buf_map_attachment(attachment, DMA_BIDIRECTIONAL);
		if (!sg_table) {
			DRM_ERROR("%s: Failed to get dma sg", __func__);
			dma_buf_detach(dmabuf, attachment);
			return -ENOMEM;
		}
		meson_gem_obj->sg = sg_table;
		meson_gem_obj->attachment = attachment;

		sg_dma_address(sg_table->sgl) = sg_phys(sg_table->sgl);
		page = sg_page(sg_table->sgl);

		if (from_heap_codecmm)
			zap_dma_buf_range(page, meson_gem_obj->base.size);

		dma_sync_sg_for_device(meson_gem_obj->base.dev->dev,
					   sg_table->sgl,
					   sg_table->nents,
					   DMA_TO_DEVICE);
		meson_gem_obj->addr = PFN_PHYS(page_to_pfn(page));
	}
#endif
#ifdef CONFIG_AMLOGIC_ION_DEV
	if (!meson_gem_obj->is_dma) {
		if (dmabuf)
			buffer = (struct ion_buffer *)dmabuf->priv;
		else
			return -ENOMEM;
		meson_gem_obj->ionbuffer = buffer;
		sg_dma_address(buffer->sg_table->sgl) = sg_phys(buffer->sg_table->sgl);
		dma_sync_sg_for_device(meson_gem_obj->base.dev->dev,
				       buffer->sg_table->sgl,
				       buffer->sg_table->nents,
				       DMA_TO_DEVICE);
		meson_ion_buffer_to_phys(buffer,
					 (phys_addr_t *)&meson_gem_obj->addr,
					 (size_t *)&len);
		meson_gem_obj->bscatter = bscatter;
		DRM_DEBUG("sg_table:nents=%d,dma_addr=0x%x,length=%d,offset=%d\n",
			  buffer->sg_table->nents,
			  (u32)buffer->sg_table->sgl->dma_address,
			  buffer->sg_table->sgl->length,
			  buffer->sg_table->sgl->offset);
		DRM_DEBUG("allocate size (%d) addr=0x%x.\n",
			  (u32)meson_gem_obj->base.size, (u32)meson_gem_obj->addr);
	}
#endif

	return 0;
}

static void am_meson_gem_free_ion_buf(struct drm_device *dev,
				      struct am_meson_gem_object *meson_gem_obj)
{
#ifdef CONFIG_AMLOGIC_ION
	if (meson_gem_obj->ionbuffer) {
		DRM_DEBUG("%s free buffer  (0x%p).\n", __func__,
			  meson_gem_obj->ionbuffer);
		dma_buf_put(meson_gem_obj->dmabuf);
		meson_gem_obj->ionbuffer = NULL;
		meson_gem_obj->dmabuf = NULL;
	}
#endif
#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
	if (meson_gem_obj->is_dma) {
		DRM_DEBUG("%s dma_heap_buffer_free  (0x%px).\n", __func__,
			  meson_gem_obj->dmabuf);
		dma_buf_unmap_attachment(meson_gem_obj->attachment,
					 meson_gem_obj->sg, DMA_BIDIRECTIONAL);
		dma_buf_detach(meson_gem_obj->dmabuf, meson_gem_obj->attachment);
		dma_buf_put(meson_gem_obj->dmabuf);
		meson_gem_obj->dmabuf = NULL;
	}
#endif
	if (!meson_gem_obj->ionbuffer && !meson_gem_obj->is_dma)
		DRM_ERROR("meson_gem_obj buffer is null\n");
}

static int am_meson_gem_alloc_video_secure_buff(struct am_meson_gem_object *meson_gem_obj)
{
	unsigned long addr;

	if (!meson_gem_obj)
		return -EINVAL;
	addr = codec_mm_alloc_for_dma(MESON_GEM_NAME,
				      meson_gem_obj->base.size / PAGE_SIZE,
				      0, CODEC_MM_FLAGS_TVP);
	if (!addr) {
		DRM_ERROR("alloc %d secure memory FAILED.\n",
			  (unsigned int)meson_gem_obj->base.size);
		return -ENOMEM;
	}
	meson_gem_obj->addr = addr;
	meson_gem_obj->is_secure = true;
	meson_gem_obj->is_uvm = true;
	DRM_DEBUG("allocate secure addr (%p).\n", &meson_gem_obj->addr);
	return 0;
}

static void am_meson_gem_free_video_secure_buf(struct am_meson_gem_object *meson_gem_obj)
{
	if (!meson_gem_obj || !meson_gem_obj->addr) {
		DRM_ERROR("meson_gem_obj or addr is null.\n");
		return;
	}
	codec_mm_free_for_dma(MESON_GEM_NAME, meson_gem_obj->addr);
	DRM_DEBUG("free secure addr (%p).\n", &meson_gem_obj->addr);
}

void meson_gem_object_free(struct drm_gem_object *obj)
{
	struct am_meson_gem_object *meson_gem_obj = to_am_meson_gem_obj(obj);

	DRM_DEBUG("%s %p handle count = %d\n", __func__, meson_gem_obj,
		  obj->handle_count);

	if ((meson_gem_obj->flags & MESON_USE_VIDEO_PLANE) &&
	    (meson_gem_obj->flags & MESON_USE_PROTECTED))
		am_meson_gem_free_video_secure_buf(meson_gem_obj);
	else if (!obj->import_attach)
		am_meson_gem_free_ion_buf(obj->dev, meson_gem_obj);
	else
		drm_prime_gem_destroy(obj, meson_gem_obj->sg);

	drm_gem_free_mmap_offset(obj);

	/* release file pointer to gem object. */
	drm_gem_object_release(obj);

	kfree(meson_gem_obj);
	meson_gem_obj = NULL;
}

static void am_meson_drm_gem_unref_uvm(struct uvm_buf_obj *ubo)
{
	struct am_meson_gem_object *meson_gem_obj;

	meson_gem_obj = uvm_to_gem_obj(ubo);

	drm_gem_object_put(&meson_gem_obj->base);
}

static struct sg_table *am_meson_gem_create_sg_table(struct drm_gem_object *obj)
{
	struct am_meson_gem_object *meson_gem_obj;
	struct sg_table *dst_table = NULL;
	struct scatterlist *dst_sg = NULL;
#ifdef CONFIG_AMLOGIC_ION
	struct sg_table *src_table = NULL;
	struct scatterlist *src_sg = NULL;
#endif
	struct page *gem_page = NULL;
	int ret;

	meson_gem_obj = to_am_meson_gem_obj(obj);

	if ((meson_gem_obj->flags & MESON_USE_VIDEO_PLANE) &&
	    (meson_gem_obj->flags & MESON_USE_PROTECTED)) {
		gem_page = phys_to_page(meson_gem_obj->addr);
		dst_table = kmalloc(sizeof(*dst_table), GFP_KERNEL);
		if (!dst_table) {
			ret = -ENOMEM;
			return ERR_PTR(ret);
		}
		ret = sg_alloc_table(dst_table, 1, GFP_KERNEL);
		if (ret) {
			kfree(dst_table);
			return ERR_PTR(ret);
		}
		dst_sg = dst_table->sgl;
		sg_set_page(dst_sg, gem_page, obj->size, 0);
		sg_dma_address(dst_sg) = meson_gem_obj->addr;
		sg_dma_len(dst_sg) = obj->size;

		return dst_table;
	} else if (meson_gem_obj->is_afbc) {
#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
		if (meson_gem_obj->is_dma)
			return meson_gem_obj->sg;
#endif
#ifdef CONFIG_AMLOGIC_ION
		if (meson_gem_obj->ionbuffer) {
			src_table = meson_gem_obj->ionbuffer->sg_table;
			dst_table = kmalloc(sizeof(*dst_table), GFP_KERNEL);
			if (!dst_table) {
				ret = -ENOMEM;
				return ERR_PTR(ret);
			}

			ret = sg_alloc_table(dst_table, 1, GFP_KERNEL);
			if (ret) {
				kfree(dst_table);
				return ERR_PTR(ret);
			}

			dst_sg = dst_table->sgl;
			src_sg = src_table->sgl;

			sg_set_page(dst_sg, sg_page(src_sg), obj->size, 0);
			sg_dma_address(dst_sg) = sg_phys(src_sg);
			sg_dma_len(dst_sg) = obj->size;

			return dst_table;
		}
#endif
	}
	DRM_ERROR("Not support import buffer from other driver.\n");
	return NULL;
}

static struct dma_buf *meson_gem_prime_export(struct drm_gem_object *obj,
					      int flags)
{
	struct dma_buf *dmabuf;
	struct am_meson_gem_object *meson_gem_obj;
	struct uvm_alloc_info info;

	meson_gem_obj = to_am_meson_gem_obj(obj);
	memset(&info, 0, sizeof(struct uvm_alloc_info));

	if (meson_gem_obj->is_uvm) {
		dmabuf = uvm_alloc_dmabuf(obj->size, 0, 0);
		if (dmabuf) {
			if (meson_gem_obj->is_afbc || meson_gem_obj->is_secure) {
				info.sgt =
				am_meson_gem_create_sg_table(obj);
			} else {
#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
				if (meson_gem_obj->is_dma)
					info.sgt =
					meson_gem_obj->sg;
#endif
#ifdef CONFIG_AMLOGIC_ION
				if (meson_gem_obj->ionbuffer)
					info.sgt =
					meson_gem_obj->ionbuffer->sg_table;
#endif
			}

			if (meson_gem_obj->is_afbc)
				info.flags |= BIT(UVM_FAKE_ALLOC);

			if (meson_gem_obj->is_secure)
				info.flags |= BIT(UVM_SECURE_ALLOC);

			info.obj = &meson_gem_obj->ubo;
			info.free = am_meson_drm_gem_unref_uvm;
			dmabuf_bind_uvm_alloc(dmabuf, &info);
			drm_gem_object_get(obj);

			if (meson_gem_obj->is_afbc ||
			    meson_gem_obj->is_secure) {
				sg_free_table(info.sgt);
				vfree(info.sgt);
			}
		}

		return dmabuf;
	}

	return drm_gem_prime_export(obj, flags);
}

static struct sg_table *meson_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct am_meson_gem_object *meson_gem_obj;
#ifdef CONFIG_AMLOGIC_ION
	struct sg_table *dst_table = NULL;
	struct scatterlist *dst_sg = NULL;
	struct sg_table *src_table = NULL;
	struct scatterlist *src_sg = NULL;
	int ret, i;
#endif

	meson_gem_obj = to_am_meson_gem_obj(obj);
	DRM_DEBUG("%s %p.\n", __func__, meson_gem_obj);

#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
	if (!meson_gem_obj->base.import_attach && meson_gem_obj->is_dma) {
		src_table = meson_gem_obj->sg;
		dst_table = kmalloc(sizeof(*dst_table), GFP_KERNEL);
		if (!dst_table) {
			ret = -ENOMEM;
			return ERR_PTR(ret);
		}

		ret = sg_alloc_table(dst_table, src_table->nents, GFP_KERNEL);
		if (ret) {
			kfree(dst_table);
			return ERR_PTR(ret);
		}

		dst_sg = dst_table->sgl;
		src_sg = src_table->sgl;
		for (i = 0; i < src_table->nents; i++) {
			sg_set_page(dst_sg, sg_page(src_sg), src_sg->length, 0);
			sg_dma_address(dst_sg) = sg_phys(src_sg);
			sg_dma_len(dst_sg) = sg_dma_len(src_sg);
			dst_sg = sg_next(dst_sg);
			src_sg = sg_next(src_sg);
		}
		return dst_table;
	}
#endif
	if (!meson_gem_obj->base.import_attach && meson_gem_obj->ionbuffer) {
		src_table = meson_gem_obj->ionbuffer->sg_table;
		dst_table = kmalloc(sizeof(*dst_table), GFP_KERNEL);
		if (!dst_table) {
			ret = -ENOMEM;
			return ERR_PTR(ret);
		}

		ret = sg_alloc_table(dst_table, src_table->nents, GFP_KERNEL);
		if (ret) {
			kfree(dst_table);
			return ERR_PTR(ret);
		}
	}
	DRM_ERROR("Not support import buffer from other driver.\n");
	return NULL;
}

static int meson_gem_prime_vmap(struct drm_gem_object *obj, struct dma_buf_map *map)
{
	struct am_meson_gem_object *meson_gem_obj = to_am_meson_gem_obj(obj);

	if (meson_gem_obj->is_uvm) {
		DRM_ERROR("UVM cannot call vmap.\n");
		return 0;
	}
	DRM_DEBUG("%s %p.\n", __func__, obj);

	return 0;
}

static void meson_gem_prime_vunmap(struct drm_gem_object *obj, struct dma_buf_map *map)
{
	DRM_DEBUG("%s nothing to do.\n", __func__);
}

#ifdef CONFIG_AMLOGIC_ION
int am_meson_gem_object_mmap(struct am_meson_gem_object *obj,
			     struct vm_area_struct *vma)
{
	int ret = 0;
	struct ion_buffer *buffer = obj->ionbuffer;
	struct ion_heap *heap = buffer->heap;

	/*
	 * Clear the VM_PFNMAP flag that was set by drm_gem_mmap(), and set the
	 * vm_pgoff (used as a fake buffer offset by DRM) to 0 as we want to map
	 * the whole buffer.
	 */
	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;

	if (obj->base.import_attach) {
		DRM_ERROR("Not support import buffer from other driver.\n");
	} else {
		if (!(buffer->flags & ION_FLAG_CACHED))
			vma->vm_page_prot =
				pgprot_writecombine(vma->vm_page_prot);

		mutex_lock(&buffer->lock);
		/* now map it to userspace */
		ret = ion_heap_map_user(heap, buffer, vma);
		mutex_unlock(&buffer->lock);
	}

	if (ret) {
		DRM_ERROR("failure mapping buffer to userspace (%d)\n", ret);
		drm_gem_vm_close(vma);
	}

	return ret;
}
#endif

#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
static int am_meson_gem_object_mmap_dma(struct am_meson_gem_object *meson_gem_obj,
	struct vm_area_struct *vma)
{
	struct sg_table *sg_table = meson_gem_obj->sg;
	unsigned long addr = vma->vm_start;
	struct sg_page_iter piter;
	int ret;

	/*
	 * Clear the VM_PFNMAP flag that was set by drm_gem_mmap(), and set the
	 * vm_pgoff (used as a fake buffer offset by DRM) to 0 as we want to map
	 * the whole buffer.
	 */
	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	for_each_sgtable_page(sg_table, &piter, vma->vm_pgoff) {
		struct page *page = sg_page_iter_page(&piter);

		ret = remap_pfn_range(vma, addr, page_to_pfn(page), PAGE_SIZE,
				      vma->vm_page_prot);
		if (ret)
			return ret;
		addr += PAGE_SIZE;
		if (addr >= vma->vm_end)
			return 0;
	}
	return 0;
}
#endif

int meson_gem_prime_mmap(struct drm_gem_object *obj,
			    struct vm_area_struct *vma)
{
	struct am_meson_gem_object *meson_gem_obj;

	meson_gem_obj = to_am_meson_gem_obj(obj);
	DRM_DEBUG("%s %p.\n", __func__, meson_gem_obj);
#if (defined CONFIG_AMLOGIC_HEAP_CMA) || (defined CONFIG_AMLOGIC_HEAP_CODEC_MM)
	if (meson_gem_obj->is_dma)
		return am_meson_gem_object_mmap_dma(meson_gem_obj, vma);
#endif
#ifdef CONFIG_AMLOGIC_ION
	if (meson_gem_obj->ionbuffer)
		return am_meson_gem_object_mmap(meson_gem_obj, vma);
#endif
	DRM_ERROR("%s: fail.\n", __func__);
	return -ENOMEM;
}

static const struct drm_gem_object_funcs meson_gem_object_funcs = {
	.free = meson_gem_object_free,
	.export = meson_gem_prime_export,
	.get_sg_table = meson_gem_prime_get_sg_table,
	.vmap = meson_gem_prime_vmap,
	.vunmap = meson_gem_prime_vunmap,
	.mmap = meson_gem_prime_mmap,
	.vm_ops = &drm_gem_cma_vm_ops,
};

struct am_meson_gem_object *am_meson_gem_object_create(struct drm_device *dev,
	unsigned int flags,
	unsigned long size)
{
	struct am_meson_gem_object *meson_gem_obj = NULL;
	int ret;

	if (!size) {
		DRM_ERROR("invalid size.\n");
		return ERR_PTR(-EINVAL);
	}

	size = roundup(size, PAGE_SIZE);
	meson_gem_obj = kzalloc(sizeof(*meson_gem_obj), GFP_KERNEL);
	if (!meson_gem_obj)
		return ERR_PTR(-ENOMEM);

	meson_gem_obj->base.funcs = &meson_gem_object_funcs;

	ret = drm_gem_object_init(dev, &meson_gem_obj->base, size);
	if (ret < 0) {
		DRM_ERROR("failed to initialize gem object\n");
		goto error;
	}

	if ((flags & MESON_USE_VIDEO_PLANE) && (flags & MESON_USE_PROTECTED))
		ret = am_meson_gem_alloc_video_secure_buff(meson_gem_obj);
	else
		ret = am_meson_gem_alloc_ion_buff(meson_gem_obj, flags);
	if (ret < 0) {
		drm_gem_object_release(&meson_gem_obj->base);
		goto error;
	}

	if (meson_gem_obj->is_uvm) {
		meson_gem_obj->ubo.arg = meson_gem_obj;
		meson_gem_obj->ubo.dev = dev->dev;
	}
	/*for release check*/
	meson_gem_obj->flags = flags;

	return meson_gem_obj;

error:
	kfree(meson_gem_obj);
	return ERR_PTR(ret);
}

phys_addr_t am_meson_gem_object_get_phyaddr(struct meson_drm *drm,
	struct am_meson_gem_object *meson_gem,
	size_t *len)
{
	*len = meson_gem->base.size;
	return meson_gem->addr;
}
EXPORT_SYMBOL(am_meson_gem_object_get_phyaddr);

int am_meson_gem_dumb_create(struct drm_file *file_priv,
			     struct drm_device *dev,
			     struct drm_mode_create_dumb *args)
{
	int ret = 0;
	struct am_meson_gem_object *meson_gem_obj;
	int min_pitch = DIV_ROUND_UP(args->width * args->bpp, 8);

	args->pitch = ALIGN(min_pitch, 64);
	if (args->size < args->pitch * args->height)
		args->size = args->pitch * args->height;

	args->size = round_up(args->size, PAGE_SIZE);
	meson_gem_obj = am_meson_gem_object_create(dev,
						   args->flags, args->size);
	if (IS_ERR(meson_gem_obj))
		return PTR_ERR(meson_gem_obj);

	/*
	 * allocate a id of idr table where the obj is registered
	 * and handle has the id what user can see.
	 */
	ret = drm_gem_handle_create(file_priv,
				    &meson_gem_obj->base, &args->handle);
	/* drop reference from allocate - handle holds it now. */
	drm_gem_object_put(&meson_gem_obj->base);
	if (ret) {
		DRM_ERROR("%s: create dumb handle failed %d\n",
			  __func__, ret);
		return ret;
	}

	DRM_DEBUG("%s: create dumb %p  with gem handle (0x%x)\n",
		  __func__, meson_gem_obj, args->handle);
	return 0;
}

int am_meson_gem_dumb_destroy(struct drm_file *file,
			      struct drm_device *dev, uint32_t handle)
{
	DRM_DEBUG("%s: destroy dumb with handle (0x%x)\n", __func__, handle);
	drm_gem_handle_delete(file, handle);
	return 0;
}

int am_meson_gem_dumb_map_offset(struct drm_file *file_priv,
				 struct drm_device *dev,
				 u32 handle, uint64_t *offset)
{
	struct drm_gem_object *obj;
	int ret = 0;

	mutex_lock(&dev->struct_mutex);

	/*
	 * get offset of memory allocated for drm framebuffer.
	 * - this callback would be called by user application
	 *	with DRM_IOCTL_MODE_MAP_DUMB command.
	 */
	obj = drm_gem_object_lookup(file_priv, handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		ret = -EINVAL;
		goto unlock;
	}

	ret = drm_gem_create_mmap_offset(obj);
	if (ret)
		goto out;

	*offset = drm_vma_node_offset_addr(&obj->vma_node);
	DRM_DEBUG("offset = 0x%lx\n", (unsigned long)*offset);

out:
	drm_gem_object_put(obj);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int am_meson_gem_create_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct am_meson_gem_object *meson_gem_obj;
	struct drm_meson_gem_create *args = data;
	int ret = 0;

	meson_gem_obj = am_meson_gem_object_create(dev, args->flags,
						   args->size);
	if (IS_ERR(meson_gem_obj))
		return PTR_ERR(meson_gem_obj);

	/*
	 * allocate a id of idr table where the obj is registered
	 * and handle has the id what user can see.
	 */
	ret = drm_gem_handle_create(file_priv,
				    &meson_gem_obj->base, &args->handle);
	/* drop reference from allocate - handle holds it now. */
	drm_gem_object_put(&meson_gem_obj->base);
	if (ret) {
		DRM_ERROR("%s: create dumb handle failed %d\n",
			  __func__, ret);
		return ret;
	}

	DRM_DEBUG("%s: create dumb %p  with gem handle (0x%x)\n",
		  __func__, meson_gem_obj, args->handle);
	return 0;
}

int am_meson_gem_create(struct meson_drm  *drmdrv)
{
	/*TODO??*/
	return 0;
}

void am_meson_gem_cleanup(struct meson_drm  *drmdrv)
{
	/*TODO??s*/
}

struct drm_gem_object *
am_meson_gem_prime_import_sg_table(struct drm_device *dev,
				   struct dma_buf_attachment *attach,
				   struct sg_table *sgt)
{
	struct am_meson_gem_object *meson_gem_obj;
	int ret;

	meson_gem_obj = kzalloc(sizeof(*meson_gem_obj), GFP_KERNEL);
	if (!meson_gem_obj)
		return ERR_PTR(-ENOMEM);

	meson_gem_obj->base.funcs = &meson_gem_object_funcs;

	ret = drm_gem_object_init(dev,
				  &meson_gem_obj->base,
			attach->dmabuf->size);
	if (ret < 0) {
		DRM_ERROR("failed to initialize gem object\n");
		kfree(meson_gem_obj);
		return  ERR_PTR(-ENOMEM);
	}

	DRM_DEBUG("%s: %p, sg_table %p\n", __func__, meson_gem_obj, sgt);
	meson_gem_obj->sg = sgt;
	meson_gem_obj->addr = sg_dma_address(sgt->sgl);
	return &meson_gem_obj->base;
}



struct drm_gem_object *am_meson_drm_gem_prime_import(struct drm_device *dev,
						     struct dma_buf *dmabuf)
{
	if (dmabuf_is_uvm(dmabuf)) {
		struct uvm_handle *handle;
		struct uvm_buf_obj *ubo;
		struct am_meson_gem_object *meson_gem_obj;

		handle = dmabuf->priv;
		ubo = handle->ua->obj;
		meson_gem_obj = uvm_to_gem_obj(ubo);

		if (ubo->dev == dev->dev) {
			drm_gem_object_get(&meson_gem_obj->base);
			return &meson_gem_obj->base;
		}
	}

	return drm_gem_prime_import(dev, dmabuf);
}

