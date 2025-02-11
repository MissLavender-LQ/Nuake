#include "ThumbnailManager.h"

#include <src/Vendors/glm/ext/matrix_clip_space.hpp>
#include <src/Rendering/SceneRenderer.h>
#include <src/Resource/ResourceLoader.h>
#include <GL\glew.h>


ThumbnailManager::ThumbnailManager()
{
	using namespace Nuake;

	m_Framebuffer = CreateRef<FrameBuffer>(false, m_ThumbnailSize);
	auto texture = CreateRef<Texture>(m_ThumbnailSize, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT);
	m_Framebuffer->SetTexture(texture, GL_DEPTH_ATTACHMENT);
	m_Framebuffer->SetTexture(CreateRef<Texture>(m_ThumbnailSize, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE), GL_COLOR_ATTACHMENT0); // Albedo
	m_Framebuffer->SetTexture(CreateRef<Texture>(m_ThumbnailSize, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE), GL_COLOR_ATTACHMENT1); //
	m_Framebuffer->SetTexture(CreateRef<Texture>(m_ThumbnailSize, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE), GL_COLOR_ATTACHMENT2); // Material + unlit
	m_Framebuffer->SetTexture(CreateRef<Texture>(m_ThumbnailSize, GL_RED_INTEGER, GL_R32I, GL_INT), GL_COLOR_ATTACHMENT3);
	m_Framebuffer->SetTexture(CreateRef<Texture>(m_ThumbnailSize, GL_RED, GL_R16F, GL_FLOAT), GL_COLOR_ATTACHMENT4); // Emissive

	//m_Framebuffer->QueueResize(m_ThumbnailSize);

	m_ShadedFramebuffer = CreateRef<FrameBuffer>(true, m_ThumbnailSize);
	m_ShadedFramebuffer->SetTexture(CreateRef<Texture>(m_ThumbnailSize, GL_RGB, GL_RGB16F, GL_FLOAT));
}

ThumbnailManager& ThumbnailManager::Get()
{
	static ThumbnailManager instance;
	return instance;
}

bool ThumbnailManager::IsThumbnailLoaded(const std::string& path) const
{
	return m_Thumbnails.find(path) != m_Thumbnails.end();
}

Ref<Nuake::Texture> ThumbnailManager::GetThumbnail(const std::string& path)
{
	if (IsThumbnailLoaded(path))
	{
		return m_Thumbnails[path];
	}
	using namespace Nuake;

	// Generate Thumbnail
	Ref<Texture> thumbnail = CreateRef<Texture>(m_ThumbnailSize, GL_RGB, GL_RGB16F, GL_FLOAT);
	GenerateThumbnail(path, thumbnail);

	m_Thumbnails[path] = thumbnail;
	return thumbnail;
}

void ThumbnailManager::MarkThumbnailAsDirty(const std::string & path)
{
	if (IsThumbnailLoaded(path))
	{
		m_Thumbnails.erase(path);
	}
}

Ref<Nuake::Texture> ThumbnailManager::GenerateThumbnail(const std::string& path, Ref<Nuake::Texture> texture)
{
	using namespace Nuake;

	const Matrix4 ortho = glm::orthoLH(-0.6f, 0.6f, -0.6f, 0.6f, -100.0f, 100.0f);
	Matrix4 view = Matrix4(1.0f);
	view = glm::lookAt(Vector3(1, -1.0, 0), Vector3(0, 0, 0), Vector3(0, 1, 0));;
	
	// Gbuffer pass
	m_Framebuffer->Bind();
	{
		RenderCommand::SetClearColor({ 0.2, 0.2, 0.2, 0.0f });
		m_Framebuffer->Clear();

		RenderCommand::Disable(RendererEnum::BLENDING);
		RenderCommand::Enable(RendererEnum::DEPTH_TEST);
		auto shader = ShaderManager::GetShader("Resources/Shaders/gbuffer.shader");
		shader->Bind();

		auto cam = Engine::GetCurrentScene()->GetCurrentCamera();
		shader->SetUniformMat4f("u_View", view);
		shader->SetUniformMat4f("u_Projection", ortho);
		shader->SetUniformMat4f("u_Model", Matrix4(1.0f));
		Ref<Material> material = ResourceLoader::LoadMaterial(path);
		material->Bind(shader);
		Renderer::SphereMesh->Draw(shader, false);
	}
	m_Framebuffer->Unbind();
	m_ShadedFramebuffer->SetTexture(texture);
	m_ShadedFramebuffer->Bind();
	{
		//RenderCommand::Enable(RendererEnum::BLENDING);
		RenderCommand::SetClearColor({ 0.2, 0.2, 0.2, 1 });
		m_ShadedFramebuffer->Clear();
		RenderCommand::Disable(RendererEnum::DEPTH_TEST);
		RenderCommand::Disable(RendererEnum::FACE_CULL);
		auto shader = ShaderManager::GetShader("Resources/Shaders/deferred.shader");
		shader->Bind();
		shader->SetUniformVec3("u_EyePosition", Vector3(1, 0, 0));
		shader->SetUniform1i("LightCount", 0);
		auto dir = Engine::GetCurrentScene()->GetEnvironment()->ProceduralSkybox->GetSunDirection();
		shader->SetUniform3f("u_DirectionalLight.Direction", 0.6, -0.6, 0.6);
		shader->SetUniform3f("u_DirectionalLight.Color", 10.0f, 10.0f, 10.0f);
		shader->SetUniform1i("u_DirectionalLight.Shadow", 0);
		shader->SetUniform1i("u_DisableSSAO", 1);
		shader->SetUniformMat4f("u_View", view);
		shader->SetUniformMat4f("u_Projection", ortho);

		m_Framebuffer->GetTexture(GL_DEPTH_ATTACHMENT)->Bind(5);
		m_Framebuffer->GetTexture(GL_COLOR_ATTACHMENT0)->Bind(6);
		m_Framebuffer->GetTexture(GL_COLOR_ATTACHMENT1)->Bind(7);
		m_Framebuffer->GetTexture(GL_COLOR_ATTACHMENT2)->Bind(8);
		m_Framebuffer->GetTexture(GL_COLOR_ATTACHMENT4)->Bind(10);

		shader->SetUniform1i("m_Depth", 5);
		shader->SetUniform1i("m_Albedo", 6);
		shader->SetUniform1i("m_Normal", 7);
		shader->SetUniform1i("m_Material", 8);
		shader->SetUniform1i("m_Emissive", 10);
	
		Renderer::DrawQuad(Matrix4());
	}
	m_ShadedFramebuffer->Unbind();

	return m_ShadedFramebuffer->GetTexture(GL_COLOR_ATTACHMENT0);
}