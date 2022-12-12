#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"

namespace CGHomework3Particles
{
	using namespace DirectX;

#define MAX_PARTICLES 15000

	struct PARTICLE_VERTEX
	{
		XMFLOAT4 Color;
	};


	struct CB_GS
	{
		XMFLOAT4X4 m_WorldViewProj;
		XMFLOAT4X4 m_InvView;
	};

	struct CB_CS
	{
		UINT param[4];
		float paramf[4];
	};

	struct PARTICLE
	{
		XMFLOAT4 pos;
		XMFLOAT4 velo;
	};

	// 此示例呈现器实例化一个基本渲染管道。
	class Sample3DSceneRenderer
	{

	public:
		float m_fSpread = 400.0f;
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		void StartTracking();
		void TrackingUpdate(float positionX);
		void StopTracking();
		bool IsTracking() { return m_tracking; }


		float RPercent();
		void CreateParticleBuffer();
		void LoadParticles(PARTICLE* pParticles, XMFLOAT3 Center, XMFLOAT4 Velocity, float Spread, UINT NumParticles);
		void CreateParticlePosVeloBuffers();
		bool RenderParticles(ID3D11DeviceContext* pd3dImmediateContext, CXMMATRIX mView, CXMMATRIX mProj);
		void Sample3DSceneRenderer::OnFrameMove();

	private:
		void Rotate(float radians);

	private:
		// 缓存的设备资源指针。
		std::shared_ptr<DX::DeviceResources> m_deviceResources;


		//For Particles
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		m_pRenderParticlesVS;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>	m_pRenderParticlesGS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		m_pRenderParticlesPS;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>		m_pSampleStateLinear;
		Microsoft::WRL::ComPtr<ID3D11BlendState>		m_pBlendingStateParticle;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_pDepthStencilState;

		Microsoft::WRL::ComPtr<ID3D11ComputeShader>		m_pCalcCS;
		Microsoft::WRL::ComPtr<ID3D11Buffer>			m_pcbCS;

		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_pParticlePosVelo0;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_pParticlePosVelo1;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_pParticlePosVeloRV0;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_pParticlePosVeloRV1;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	m_pParticlePosVeloUAV0;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	m_pParticlePosVeloUAV1;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_pParticleBuffer;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>			m_pParticleVertexLayout;


		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_pcbGS;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_pParticleTexRV;


		// 立体几何的系统资源。
		ModelViewProjectionConstantBuffer	m_constantBufferData;

		// 用于渲染循环的变量。
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;
	};
}

