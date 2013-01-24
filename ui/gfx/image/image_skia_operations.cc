// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/image/image_skia_operations.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "skia/ext/image_operations.h"
#include "ui/base/layout.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/gfx/image/image_skia_source.h"
#include "ui/gfx/insets.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/rect_conversions.h"
#include "ui/gfx/size.h"
#include "ui/gfx/size_conversions.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/gfx/skia_util.h"

namespace gfx {
namespace {

// Returns an image rep for the ImageSkiaSource to return to visually indicate
// an error.
ImageSkiaRep GetErrorImageRep(ui::ScaleFactor scale_factor,
                              const gfx::Size& pixel_size) {
  SkBitmap bitmap;
  bitmap.setConfig(
      SkBitmap::kARGB_8888_Config, pixel_size.width(), pixel_size.height());
  bitmap.allocPixels();
  bitmap.eraseColor(SK_ColorRED);
  return gfx::ImageSkiaRep(bitmap, scale_factor);
}

// A base image source class that creates an image from two source images.
// This class guarantees that two ImageSkiaReps have have the same pixel size.
class BinaryImageSource : public gfx::ImageSkiaSource {
 protected:
  BinaryImageSource(const ImageSkia& first,
                    const ImageSkia& second,
                    const char* source_name)
      : first_(first),
        second_(second),
        source_name_(source_name) {
  }
  virtual ~BinaryImageSource() {
  }

  // gfx::ImageSkiaSource overrides:
  virtual ImageSkiaRep GetImageForScale(ui::ScaleFactor scale_factor) OVERRIDE {
    ImageSkiaRep first_rep = first_.GetRepresentation(scale_factor);
    ImageSkiaRep second_rep = second_.GetRepresentation(scale_factor);
    if (first_rep.pixel_width() != second_rep.pixel_width() ||
        first_rep.pixel_height() != second_rep.pixel_height()) {
      DCHECK_NE(first_rep.scale_factor(), second_rep.scale_factor());
      if (first_rep.scale_factor() == second_rep.scale_factor()) {
        LOG(ERROR) << "ImageSkiaRep size mismatch in " << source_name_;
        return GetErrorImageRep(first_rep.scale_factor(),
                                gfx::Size(first_rep.pixel_width(),
                                          first_rep.pixel_height()));
      }
      first_rep = first_.GetRepresentation(ui::SCALE_FACTOR_100P);
      second_rep = second_.GetRepresentation(ui::SCALE_FACTOR_100P);
      DCHECK_EQ(first_rep.pixel_width(), second_rep.pixel_width());
      DCHECK_EQ(first_rep.pixel_height(), second_rep.pixel_height());
      if (first_rep.pixel_width() != second_rep.pixel_width() ||
          first_rep.pixel_height() != second_rep.pixel_height()) {
        LOG(ERROR) << "ImageSkiaRep size mismatch in " << source_name_;
        return GetErrorImageRep(first_rep.scale_factor(),
                                gfx::Size(first_rep.pixel_width(),
                                          first_rep.pixel_height()));
      }
    } else {
      DCHECK_EQ(first_rep.scale_factor(), second_rep.scale_factor());
    }
    return CreateImageSkiaRep(first_rep, second_rep);
  }

  // Creates a final image from two ImageSkiaReps. The pixel size of
  // the two images are guaranteed to be the same.
  virtual ImageSkiaRep CreateImageSkiaRep(
      const ImageSkiaRep& first_rep,
      const ImageSkiaRep& second_rep) const = 0;

 private:
  const ImageSkia first_;
  const ImageSkia second_;
  // The name of a class that implements the BinaryImageSource.
  // The subclass is responsible for managing the memory.
  const char* source_name_;

  DISALLOW_COPY_AND_ASSIGN(BinaryImageSource);
};

class BlendingImageSource : public BinaryImageSource {
 public:
  BlendingImageSource(const ImageSkia& first,
                      const ImageSkia& second,
                      double alpha)
      : BinaryImageSource(first, second, "BlendingImageSource"),
        alpha_(alpha) {
  }

  virtual ~BlendingImageSource() {
  }

  // BinaryImageSource overrides:
  virtual ImageSkiaRep CreateImageSkiaRep(
      const ImageSkiaRep& first_rep,
      const ImageSkiaRep& second_rep) const OVERRIDE {
    SkBitmap blended = SkBitmapOperations::CreateBlendedBitmap(
        first_rep.sk_bitmap(), second_rep.sk_bitmap(), alpha_);
    return ImageSkiaRep(blended, first_rep.scale_factor());
  }

 private:
  double alpha_;

  DISALLOW_COPY_AND_ASSIGN(BlendingImageSource);
};

class SuperimposedImageSource : public gfx::CanvasImageSource {
 public:
  SuperimposedImageSource(const ImageSkia& first,
                          const ImageSkia& second)
      : gfx::CanvasImageSource(first.size(), false /* is opaque */),
        first_(first),
        second_(second) {
  }

  virtual ~SuperimposedImageSource() {}

  // gfx::CanvasImageSource override.
  virtual void Draw(Canvas* canvas) OVERRIDE {
    canvas->DrawImageInt(first_, 0, 0);
    canvas->DrawImageInt(second_,
                         (first_.width() - second_.width()) / 2,
                         (first_.height() - second_.height()) / 2);
  }

 private:
  const ImageSkia first_;
  const ImageSkia second_;

  DISALLOW_COPY_AND_ASSIGN(SuperimposedImageSource);
};

class TransparentImageSource : public gfx::ImageSkiaSource {
 public:
  TransparentImageSource(const ImageSkia& image, double alpha)
      : image_(image),
        alpha_(alpha) {
  }

  virtual ~TransparentImageSource() {}

 private:
  // gfx::ImageSkiaSource overrides:
  virtual ImageSkiaRep GetImageForScale(ui::ScaleFactor scale_factor) OVERRIDE {
    ImageSkiaRep image_rep = image_.GetRepresentation(scale_factor);
    SkBitmap alpha;
    alpha.setConfig(SkBitmap::kARGB_8888_Config,
                    image_rep.pixel_width(),
                    image_rep.pixel_height());
    alpha.allocPixels();
    alpha.eraseColor(SkColorSetARGB(alpha_ * 255, 0, 0, 0));
    return ImageSkiaRep(
        SkBitmapOperations::CreateMaskedBitmap(image_rep.sk_bitmap(), alpha),
        image_rep.scale_factor());
  }

  ImageSkia image_;
  double alpha_;

  DISALLOW_COPY_AND_ASSIGN(TransparentImageSource);
};

class MaskedImageSource : public BinaryImageSource {
 public:
  MaskedImageSource(const ImageSkia& rgb, const ImageSkia& alpha)
      : BinaryImageSource(rgb, alpha, "MaskedImageSource") {
  }

  virtual ~MaskedImageSource() {
  }

  // BinaryImageSource overrides:
  virtual ImageSkiaRep CreateImageSkiaRep(
      const ImageSkiaRep& first_rep,
      const ImageSkiaRep& second_rep) const OVERRIDE {
    return ImageSkiaRep(SkBitmapOperations::CreateMaskedBitmap(
        first_rep.sk_bitmap(), second_rep.sk_bitmap()),
                        first_rep.scale_factor());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MaskedImageSource);
};

class TiledImageSource : public gfx::ImageSkiaSource {
 public:
  TiledImageSource(const ImageSkia& source,
                   int src_x, int src_y,
                   int dst_w, int dst_h)
      : source_(source),
        src_x_(src_x),
        src_y_(src_y),
        dst_w_(dst_w),
        dst_h_(dst_h) {
  }

  virtual ~TiledImageSource() {
  }

  // gfx::ImageSkiaSource overrides:
  virtual ImageSkiaRep GetImageForScale(ui::ScaleFactor scale_factor) OVERRIDE {
    ImageSkiaRep source_rep = source_.GetRepresentation(scale_factor);
    float scale = ui::GetScaleFactorScale(source_rep.scale_factor());
    return ImageSkiaRep(
        SkBitmapOperations::CreateTiledBitmap(
            source_rep.sk_bitmap(),
            src_x_ * scale, src_y_ * scale, dst_w_ * scale, dst_h_ * scale),
        source_rep.scale_factor());
  }

 private:
  const ImageSkia source_;
  const int src_x_;
  const int src_y_;
  const int dst_w_;
  const int dst_h_;

  DISALLOW_COPY_AND_ASSIGN(TiledImageSource);
};

class HSLImageSource : public gfx::ImageSkiaSource {
 public:
  HSLImageSource(const ImageSkia& image,
                 const color_utils::HSL& hsl_shift)
      : image_(image),
        hsl_shift_(hsl_shift) {
  }

  virtual ~HSLImageSource() {
  }

  // gfx::ImageSkiaSource overrides:
  virtual ImageSkiaRep GetImageForScale(ui::ScaleFactor scale_factor) OVERRIDE {
    ImageSkiaRep image_rep = image_.GetRepresentation(scale_factor);
    return gfx::ImageSkiaRep(
        SkBitmapOperations::CreateHSLShiftedBitmap(image_rep.sk_bitmap(),
            hsl_shift_), image_rep.scale_factor());
  }

 private:
  const gfx::ImageSkia image_;
  const color_utils::HSL hsl_shift_;
  DISALLOW_COPY_AND_ASSIGN(HSLImageSource);
};

// ImageSkiaSource which uses SkBitmapOperations::CreateButtonBackground
// to generate image reps for the target image.
class ButtonImageSource: public gfx::ImageSkiaSource {
 public:
  ButtonImageSource(SkColor color,
                    const ImageSkia& image,
                    const ImageSkia& mask)
      : color_(color),
        image_(image),
        mask_(mask) {
  }

  virtual ~ButtonImageSource() {
  }

  // gfx::ImageSkiaSource overrides:
  virtual ImageSkiaRep GetImageForScale(ui::ScaleFactor scale_factor) OVERRIDE {
    ImageSkiaRep image_rep = image_.GetRepresentation(scale_factor);
    ImageSkiaRep mask_rep = mask_.GetRepresentation(scale_factor);
    if (image_rep.scale_factor() != mask_rep.scale_factor()) {
      image_rep = image_.GetRepresentation(ui::SCALE_FACTOR_100P);
      mask_rep = mask_.GetRepresentation(ui::SCALE_FACTOR_100P);
    }
    return gfx::ImageSkiaRep(
        SkBitmapOperations::CreateButtonBackground(color_,
              image_rep.sk_bitmap(), mask_rep.sk_bitmap()),
          image_rep.scale_factor());
  }

 private:
  const SkColor color_;
  const ImageSkia image_;
  const ImageSkia mask_;

  DISALLOW_COPY_AND_ASSIGN(ButtonImageSource);
};

// ImageSkiaSource which uses SkBitmap::extractSubset to generate image reps
// for the target image.
class ExtractSubsetImageSource: public gfx::ImageSkiaSource {
 public:
  ExtractSubsetImageSource(const gfx::ImageSkia& image,
                           const gfx::Rect& subset_bounds)
      : image_(image),
        subset_bounds_(subset_bounds) {
  }

  virtual ~ExtractSubsetImageSource() {
  }

  // gfx::ImageSkiaSource overrides:
  virtual ImageSkiaRep GetImageForScale(ui::ScaleFactor scale_factor) OVERRIDE {
    ImageSkiaRep image_rep = image_.GetRepresentation(scale_factor);
    float scale_to_pixel = ui::GetScaleFactorScale(image_rep.scale_factor());
    SkIRect subset_bounds_in_pixel = RectToSkIRect(ToFlooredRectDeprecated(
        gfx::ScaleRect(subset_bounds_, scale_to_pixel)));
    SkBitmap dst;
    bool success = image_rep.sk_bitmap().extractSubset(&dst,
                                                       subset_bounds_in_pixel);
    DCHECK(success);
    return gfx::ImageSkiaRep(dst, image_rep.scale_factor());
  }

 private:
  const gfx::ImageSkia image_;
  const gfx::Rect subset_bounds_;

  DISALLOW_COPY_AND_ASSIGN(ExtractSubsetImageSource);
};

// ResizeSource resizes relevant image reps in |source| to |target_dip_size|
// for requested scale factors.
class ResizeSource : public ImageSkiaSource {
 public:
  ResizeSource(const ImageSkia& source,
               skia::ImageOperations::ResizeMethod method,
               const Size& target_dip_size)
      : source_(source),
        resize_method_(method),
        target_dip_size_(target_dip_size) {
  }
  virtual ~ResizeSource() {}

  // gfx::ImageSkiaSource overrides:
  virtual ImageSkiaRep GetImageForScale(ui::ScaleFactor scale_factor) OVERRIDE {
    const ImageSkiaRep& image_rep = source_.GetRepresentation(scale_factor);
    if (image_rep.GetWidth() == target_dip_size_.width() &&
        image_rep.GetHeight() == target_dip_size_.height())
      return image_rep;

    const float scale = ui::GetScaleFactorScale(scale_factor);
    const Size target_pixel_size = gfx::ToFlooredSize(
        gfx::ScaleSize(target_dip_size_, scale));
    const SkBitmap resized = skia::ImageOperations::Resize(
        image_rep.sk_bitmap(),
        resize_method_,
        target_pixel_size.width(),
        target_pixel_size.height());
    return ImageSkiaRep(resized, scale_factor);
  }

 private:
  const ImageSkia source_;
  skia::ImageOperations::ResizeMethod resize_method_;
  const Size target_dip_size_;

  DISALLOW_COPY_AND_ASSIGN(ResizeSource);
};

// DropShadowSource generates image reps with drop shadow for image reps in
// |source| that represent requested scale factors.
class DropShadowSource : public ImageSkiaSource {
 public:
  DropShadowSource(const ImageSkia& source,
                   const ShadowValues& shadows_in_dip)
      : source_(source),
        shaodws_in_dip_(shadows_in_dip) {
  }
  virtual ~DropShadowSource() {}

  // gfx::ImageSkiaSource overrides:
  virtual ImageSkiaRep GetImageForScale(ui::ScaleFactor scale_factor) OVERRIDE {
    const ImageSkiaRep& image_rep = source_.GetRepresentation(scale_factor);

    const float scale = image_rep.GetScale();
    ShadowValues shadows_in_pixel;
    for (size_t i = 0; i < shaodws_in_dip_.size(); ++i)
      shadows_in_pixel.push_back(shaodws_in_dip_[i].Scale(scale));

    const SkBitmap shadow_bitmap = SkBitmapOperations::CreateDropShadow(
        image_rep.sk_bitmap(),
        shadows_in_pixel);
    return ImageSkiaRep(shadow_bitmap, image_rep.scale_factor());
  }

 private:
  const ImageSkia source_;
  const ShadowValues shaodws_in_dip_;

  DISALLOW_COPY_AND_ASSIGN(DropShadowSource);
};

// RotatedSource generates image reps that are rotations of those in
// |source| that represent requested scale factors.
class RotatedSource : public ImageSkiaSource {
 public:
  RotatedSource(const ImageSkia& source,
                SkBitmapOperations::RotationAmount rotation)
    : source_(source),
      rotation_(rotation) {
  }
  virtual ~RotatedSource() {}

  // gfx::ImageSkiaSource overrides:
  virtual ImageSkiaRep GetImageForScale(ui::ScaleFactor scale_factor) OVERRIDE {
    const ImageSkiaRep& image_rep = source_.GetRepresentation(scale_factor);
    const SkBitmap rotated_bitmap =
        SkBitmapOperations::Rotate(image_rep.sk_bitmap(), rotation_);
    return ImageSkiaRep(rotated_bitmap, image_rep.scale_factor());
  }

 private:
  const ImageSkia source_;
  const SkBitmapOperations::RotationAmount rotation_;

  DISALLOW_COPY_AND_ASSIGN(RotatedSource);
};


}  // namespace

// static
ImageSkia ImageSkiaOperations::CreateBlendedImage(const ImageSkia& first,
                                                  const ImageSkia& second,
                                                  double alpha) {
  return ImageSkia(new BlendingImageSource(first, second, alpha), first.size());
}

// static
ImageSkia ImageSkiaOperations::CreateSuperimposedImage(
    const ImageSkia& first,
    const ImageSkia& second) {
  return ImageSkia(new SuperimposedImageSource(first, second), first.size());
}

// static
ImageSkia ImageSkiaOperations::CreateTransparentImage(const ImageSkia& image,
                                                      double alpha) {
  return ImageSkia(new TransparentImageSource(image, alpha), image.size());
}

// static
ImageSkia ImageSkiaOperations::CreateMaskedImage(const ImageSkia& rgb,
                                                 const ImageSkia& alpha) {
  return ImageSkia(new MaskedImageSource(rgb, alpha), rgb.size());
}

// static
ImageSkia ImageSkiaOperations::CreateTiledImage(const ImageSkia& source,
                                                int src_x, int src_y,
                                                int dst_w, int dst_h) {
  return ImageSkia(new TiledImageSource(source, src_x, src_y, dst_w, dst_h),
                   gfx::Size(dst_w, dst_h));
}

// static
ImageSkia ImageSkiaOperations::CreateHSLShiftedImage(
    const ImageSkia& image,
    const color_utils::HSL& hsl_shift) {
  return ImageSkia(new HSLImageSource(image, hsl_shift), image.size());
}

// static
ImageSkia ImageSkiaOperations::CreateButtonBackground(SkColor color,
                                                      const ImageSkia& image,
                                                      const ImageSkia& mask) {
  return ImageSkia(new ButtonImageSource(color, image, mask), mask.size());
}

// static
ImageSkia ImageSkiaOperations::ExtractSubset(const ImageSkia& image,
                                             const Rect& subset_bounds) {
  gfx::Rect clipped_bounds =
      gfx::IntersectRects(subset_bounds, gfx::Rect(image.size()));
  if (image.isNull() || clipped_bounds.IsEmpty()) {
    return ImageSkia();
  }

  return ImageSkia(new ExtractSubsetImageSource(image, clipped_bounds),
                   clipped_bounds.size());
}

// static
ImageSkia ImageSkiaOperations::CreateResizedImage(
    const ImageSkia& source,
    skia::ImageOperations::ResizeMethod method,
    const Size& target_dip_size) {
  return ImageSkia(new ResizeSource(source, method, target_dip_size),
                   target_dip_size);
}

// static
ImageSkia ImageSkiaOperations::CreateImageWithDropShadow(
    const ImageSkia& source,
    const ShadowValues& shadows) {
  const gfx::Insets shadow_padding = -gfx::ShadowValue::GetMargin(shadows);
  gfx::Size shadow_image_size = source.size();
  shadow_image_size.Enlarge(shadow_padding.width(),
                            shadow_padding.height());
  return ImageSkia(new DropShadowSource(source, shadows), shadow_image_size);
}

// static
ImageSkia ImageSkiaOperations::CreateRotatedImage(
      const ImageSkia& source,
      SkBitmapOperations::RotationAmount rotation) {
  return ImageSkia(new RotatedSource(source, rotation),
      SkBitmapOperations::ROTATION_180_CW == rotation ?
          source.size() :
          gfx::Size(source.height(), source.width()));

}

}  // namespace gfx
