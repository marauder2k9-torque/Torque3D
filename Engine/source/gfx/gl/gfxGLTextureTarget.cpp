#include "platform/platform.h"
#include "gfx/gl/gfxGLTextureTarget.h"
#include "gfx/gl/gfxGLEnumTranslate.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxStringEnumTranslate.h"
#include "gfx/gl/gfxGLUtils.h"

GFXGLTextureTarget::GFXGLTextureTarget(bool genMips)
   :  mTargetSize(Point2I::Zero),
      mTargetFormat(GFXFormatR8G8B8A8)
{
   glGenFramebuffers(1, &mFBO);

   for (S32 i = 0; i < MaxRenderSlotId; ++i) {
      mTargets[i] = 0;
      mTargetFace[i] = 0;
      mTargetIsCube[i] = false;
   }

   mGenMips = genMips;

   glGenFramebuffers(1, &mCopyFboSrc);
   glGenFramebuffers(1, &mCopyFboDst);
}

GFXGLTextureTarget::~GFXGLTextureTarget()
{
   glDeleteFramebuffers(1, &mFBO);

   glDeleteFramebuffers(1, &mCopyFboSrc);
   glDeleteFramebuffers(1, &mCopyFboDst);
}

void GFXGLTextureTarget::attachTexture(RenderSlot slot, GFXTextureObject* tex, U32 mipLevel, U32 zOffset, U32 faceIndex)
{
   GFXDEBUGEVENT_SCOPE(GFXGLTextureTarget_attachTexture, ColorI::RED);
   AssertFatal(slot < MaxRenderSlotId, "GFXD3D11TextureTarget::attachTexture - out of range slot.");

   if (tex == GFXTextureTarget::sDefaultDepthStencil)
      tex = GFXGL->getDefaultDepthTex();

   if ((!tex))
      return;

   invalidateState();
   AssertFatal(dynamic_cast<GFXGLTextureObject*>(tex), "GFXGLTextureTarget::attachTexture - invalid texture object.");

   GFXGLTextureObject* oglto = dynamic_cast<GFXGLTextureObject*>(tex);
   AssertFatal(oglto, "attachTexture: null GFXGLTextureObject!");

   mTargets[slot] = 0;
   mTargetFace[slot] = 0;
   mTargetIsCube[slot] = false;

   if (slot == Color0)
   {
      mTargetSize = Point2I::Zero;
      mTargetFormat = GFXFormatR8G8B8A8;
   }

   // Save state
   mTargets[slot] = oglto->getHandle();
   mTargetFace[slot] = faceIndex;
   mTargetIsCube[slot] = oglto->isCubeMap();

   // Basic validation
   if (mTargetIsCube[slot])
      AssertFatal(faceIndex < 6, "attachTexture: invalid cubemap faceIndex");

   AssertFatal(mipLevel < oglto->mMipLevels, "attachTexture: mipLevel out of range");

   PRESERVE_FRAMEBUFFER();
   glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
   glEnable(GL_FRAMEBUFFER_SRGB);

   // Check handle validity
   if (!glIsTexture(oglto->getHandle()))
      Con::errorf("AttachTexture: handle %u is not a valid GL texture!", oglto->getHandle());

   // Bind the texture to the *binding* target so queries like glGetTexLevelParameteriv work.
   GLenum bindTarget = mTargetIsCube[slot] ? GL_TEXTURE_CUBE_MAP : oglto->getBinding();
   glBindTexture(bindTarget, oglto->getHandle());

   // Query the size of the level we're about to attach (debugging)
   GLenum faceTarget = mTargetIsCube[slot] ? GFXGLFaceType[faceIndex] : bindTarget;

   // Choose attachment point
   GLenum attachment = (slot == DepthStencil) ? GL_DEPTH_STENCIL_ATTACHMENT : GL_COLOR_ATTACHMENT0 + (slot);

   // IMPORTANT: glFramebufferTexture2D requires a face-specific target for cube maps
   glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, faceTarget, oglto->getHandle(), mipLevel);
   // Clears the texture (note that the binding is irrelevent)
   if (slot != DepthStencil)
   {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
   }
   else
   {
      // if depth clear the color attachment
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (slot), GL_TEXTURE_2D, 0, 0);
   }

   if (slot == Color0)
   {
      mTargetSize = Point2I(tex->getWidth(), tex->getHeight());
      mTargetFormat = tex->getFormat();
   }

   CHECK_FRAMEBUFFER_STATUS();
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void GFXGLTextureTarget::attachTexture(RenderSlot slot, GFXCubemap* tex, U32 face, U32 mipLevel)
{
}

void GFXGLTextureTarget::activate()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
   GFXGL->getOpenglCache()->setCacheBinded(GL_FRAMEBUFFER, mFBO);

   GLenum drawBuffers[MaxRenderSlotId];
   S32 count = 0;
   for (S32 i = Color0; i < MaxRenderSlotId; ++i) {
      if (mTargets[i] != 0) {
         drawBuffers[count++] = GL_COLOR_ATTACHMENT0 + (i);
      }
   }
   glDrawBuffers(count, drawBuffers);
   CHECK_FRAMEBUFFER_STATUS();
}

void GFXGLTextureTarget::deactivate()
{
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   GFXGL->getOpenglCache()->setCacheBinded(GL_FRAMEBUFFER, 0);

   if (!mGenMips)
      return;

   for (S32 i = 0; i < MaxRenderSlotId; ++i) {
      if (mTargets[i] == 0) continue;

      GLenum bindTarget = mTargetIsCube[i] ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
      PRESERVE_TEXTURE(bindTarget);
      glBindTexture(bindTarget, mTargets[i]);
      // debug: ensure it's a texture
      if (!glIsTexture(mTargets[i])) {
         Con::errorf("resolve: texture %u not valid", mTargets[i]);
         continue;
      }
      glGenerateMipmap(bindTarget);
   }
}

void GFXGLTextureTarget::zombify()
{
   invalidateState();
}

void GFXGLTextureTarget::resurrect()
{
}

void GFXGLTextureTarget::resolve()
{
}

void GFXGLTextureTarget::resolveTo(GFXTextureObject* obj)
{
   AssertFatal(dynamic_cast<GFXGLTextureObject*>(obj),
      "GFXGLTextureTarget::resolveTo - Incorrect type of texture, expected a GFXGLTextureObject");

   GFXGLTextureObject* dstTex = static_cast<GFXGLTextureObject*>(obj);

   if (!mTargets[Color0] || !glIsTexture(mTargets[Color0]))
      return;

   GLuint srcHandle = mTargets[Color0];
   GLenum srcBinding = mTargetIsCube[Color0] ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
   GLenum dstBinding = dstTex->isCubeMap() ? GL_TEXTURE_CUBE_MAP : dstTex->getBinding();

   GLint srcWidth = 0, srcHeight = 0;
   glBindTexture(srcBinding, srcHandle);

   glGetTexLevelParameteriv(srcBinding == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : srcBinding, 0, GL_TEXTURE_WIDTH, &srcWidth);
   glGetTexLevelParameteriv(srcBinding == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : srcBinding, 0, GL_TEXTURE_HEIGHT, &srcHeight);

   glBindTexture(srcBinding, 0); // freeup

   // Use glCopyImageSubData if available and compatible
   if (GFXGL->mCapabilities.copyImage &&
      ((srcBinding == GL_TEXTURE_2D && dstBinding == GL_TEXTURE_2D) ||
         (srcBinding == GL_TEXTURE_CUBE_MAP && dstBinding == GL_TEXTURE_CUBE_MAP)))
   {
      GLint srcZ = mTargetIsCube[Color0] ? mTargetFace[Color0] : 0;
      glCopyImageSubData(
         srcHandle, srcBinding, 0, 0, 0, srcZ,
         dstTex->getHandle(), dstBinding, 0, 0, 0, 0,
         srcWidth, srcHeight, 1
      );
      return;
   }

   PRESERVE_FRAMEBUFFER();

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mCopyFboDst);
   glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dstTex->getBinding(), dstTex->getHandle(), 0);

   glBindFramebuffer(GL_READ_FRAMEBUFFER, mCopyFboSrc);
   glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, srcBinding, mTargets[Color0], 0);

   glBlitFramebuffer(0, 0, srcWidth, srcHeight,
      0, 0, dstTex->getWidth(), dstTex->getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
