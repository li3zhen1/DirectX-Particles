#include "pch.h"
#include "Sample3DSceneRenderer.h"
#include <DDSTextureLoader.h>
#include "..\Common\DirectXHelper.h"

using namespace CGHomework3Particles;

using namespace DirectX;
using namespace Windows::Foundation;

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p) = nullptr; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

// 从文件中加载顶点和像素着色器，然后实例化立方体几何图形。
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// 当窗口的大小改变时初始化视图参数。
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
}

// 每个帧调用一次，旋转立方体，并计算模型和视图矩阵。
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
}

// 将 3D 立方体模型旋转一定数量的弧度。
void Sample3DSceneRenderer::Rotate(float radians)
{
}

void Sample3DSceneRenderer::StartTracking()
{
	m_tracking = true;
}

// 进行跟踪时，可跟踪指针相对于输出屏幕宽度的位置，从而让 3D 立方体围绕其 Y 轴旋转。
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
}

void Sample3DSceneRenderer::StopTracking()
{
	m_tracking = false;
}

// 使用顶点和像素着色器呈现一个帧。
void Sample3DSceneRenderer::Render()
{
	// 加载是异步的。仅在加载几何图形后才会绘制它。
	if (!m_loadingComplete)
	{
		return;
	}
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 160.0f * XM_PI / 180.0f;

	// 这是一个简单的更改示例，当应用程序在纵向视图或对齐视图中时，可以进行此更改
	//。

	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.00001f,
		50000.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);
	auto context = m_deviceResources->GetD3DDeviceContext();
	static const XMVECTORF32 eye = { -m_fSpread * 0, m_fSpread * 20, -m_fSpread * 3, 0.f };
	static const XMVECTORF32 at = { 0.0f, -m_fSpread * 30, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 1.0f, 1.0f, 0.0f, 0.0f };
	RenderParticles(context, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)), perspectiveMatrix);
	OnFrameMove();
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	// 通过异步方式加载着色器。
	auto loadVSTask = DX::ReadDataAsync(L"VertexShader.cso");
	auto loadGSTask = DX::ReadDataAsync(L"GeometryShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"PixelShader.cso");
	auto loadCSTask = DX::ReadDataAsync(L"ComputeShader.cso");

	// 加载顶点着色器文件之后，创建着色器和输入布局。
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				m_pRenderParticlesVS.GetAddressOf()
				)
			);
		const D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				layout,
				ARRAYSIZE(layout),
				&fileData[0],
				fileData.size(),
				m_pParticleVertexLayout.GetAddressOf()
				)
			);
		});
	auto createGSTask = loadGSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateGeometryShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				m_pRenderParticlesGS.GetAddressOf()
				)
			);
		});
	// 加载像素着色器文件后，创建着色器和常量缓冲区。
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				m_pRenderParticlesPS.GetAddressOf()
				)
			);
		});
	auto createCSTask = loadCSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				m_pCalcCS.GetAddressOf()
				)
			);
		});


	// 加载两个着色器后，创建网格。
	auto createCubeTask = (createPSTask && createVSTask && createGSTask && createCSTask).then([this]() {
		CreateParticleBuffer();
		CreateParticlePosVeloBuffers();

		// Setup constant buffer
		D3D11_BUFFER_DESC Desc;
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.MiscFlags = 0;
		Desc.ByteWidth = sizeof(CB_GS);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(&Desc, nullptr, m_pcbGS.GetAddressOf())
			);
		Desc.ByteWidth = sizeof(CB_CS);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(&Desc, nullptr, m_pcbCS.GetAddressOf())
			);
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(
				m_deviceResources->GetD3DDevice(),
				L"Assets\\Particle.dds",
				nullptr,
				m_pParticleTexRV.GetAddressOf())
			);


		D3D11_SAMPLER_DESC SamplerDesc = {};
		SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateSamplerState(&SamplerDesc, m_pSampleStateLinear.GetAddressOf())
			);


		D3D11_BLEND_DESC BlendStateDesc = {};
		BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBlendState(&BlendStateDesc, m_pBlendingStateParticle.GetAddressOf())
			);


		D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = {};
		DepthStencilDesc.DepthEnable = FALSE;
		DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&DepthStencilDesc, m_pDepthStencilState.GetAddressOf())
			);



		});

	createCubeTask.then([this]() {
		m_loadingComplete = true;
		});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_pRenderParticlesVS.Reset();
	m_pRenderParticlesGS.Reset();
	m_pRenderParticlesPS.Reset();
	m_pSampleStateLinear.Reset();
	m_pBlendingStateParticle.Reset();
	m_pDepthStencilState.Reset();
	m_pCalcCS.Reset();
	m_pcbCS.Reset();

	m_pParticlePosVelo0.Reset();
	m_pParticlePosVelo1.Reset();
	m_pParticlePosVeloRV0.Reset();
	m_pParticlePosVeloRV1.Reset();
	m_pParticlePosVeloUAV0.Reset();
	m_pParticlePosVeloUAV1.Reset();
	m_pParticleBuffer.Reset();
	m_pParticleVertexLayout.Reset();


	m_pcbGS.Reset();
	m_pParticleTexRV.Reset();
}




//--------------------------------------------------------------------------------------
void Sample3DSceneRenderer::CreateParticleBuffer()
{

	D3D11_BUFFER_DESC vbdesc =
	{
		MAX_PARTICLES * sizeof(PARTICLE_VERTEX),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0
	};
	D3D11_SUBRESOURCE_DATA vbInitData = {};

	auto pVertices = std::make_unique<PARTICLE_VERTEX[]>(MAX_PARTICLES);;
	if (!pVertices)
		return;// E_OUTOFMEMORY;

	for (UINT i = 0; i < MAX_PARTICLES; i++)
	{
		pVertices[i].Color = XMFLOAT4(0.3f, 0.6f, 0.3f, 1);
	}

	vbInitData.pSysMem = pVertices.get();

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vbdesc,
			&vbInitData,
			m_pParticleBuffer.GetAddressOf()
			)
		);
}


//--------------------------------------------------------------------------------------
float Sample3DSceneRenderer::RPercent()
{
	float ret = (float)((rand() % 10000) - 5000);
	return ret / 5000.0f;
}


//--------------------------------------------------------------------------------------
// This helper function loads a group of particles
//--------------------------------------------------------------------------------------
void Sample3DSceneRenderer::LoadParticles(PARTICLE* pParticles,
	XMFLOAT3 Center, XMFLOAT4 Velocity, float Spread, UINT NumParticles)
{
	XMVECTOR vCenter = XMLoadFloat3(&Center);

	for (UINT i = 0; i < NumParticles; i++)
	{
		XMVECTOR vDelta = XMVectorReplicate(Spread);

		while (XMVectorGetX(XMVector3LengthSq(vDelta)) > Spread * Spread)
		{
			vDelta = XMVectorSet(RPercent() * Spread, RPercent() * Spread, RPercent() * Spread, 0.f);
		}

		XMVECTOR vPos = XMVectorAdd(vCenter, vDelta);

		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&pParticles[i].pos), vPos);
		pParticles[i].pos.w = 10000.0f * 10000.0f;

		pParticles[i].velo = Velocity;
	}
}


//--------------------------------------------------------------------------------------
void Sample3DSceneRenderer::CreateParticlePosVeloBuffers()
{

	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth = MAX_PARTICLES * sizeof(PARTICLE);
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(PARTICLE);
	desc.Usage = D3D11_USAGE_DEFAULT;

	// Initialize the data in the buffers
	PARTICLE* pData1 = new PARTICLE[MAX_PARTICLES];
	if (!pData1)
		return; //E_OUTOFMEMORY;

	srand((unsigned int)GetTickCount64());

#if 1
	// Disk Galaxy Formation
	float fCenterSpread = m_fSpread * 0.50f;
	LoadParticles(pData1,
		XMFLOAT3(fCenterSpread, 0, 0), XMFLOAT4(0, 0, -20, 1 / 10000.0f / 10000.0f),
		m_fSpread, MAX_PARTICLES / 2);
	LoadParticles(&pData1[MAX_PARTICLES / 2],
		XMFLOAT3(-fCenterSpread, 0, 0), XMFLOAT4(0, 0, 20, 1 / 10000.0f / 10000.0f),
		m_fSpread, MAX_PARTICLES / 2);
#else
	// Disk Galaxy Formation with impacting third cluster
	LoadParticles(pData1,
		XMFLOAT3(m_fSpread, 0, 0), XMFLOAT4(0, 0, -8, 1 / 10000.0f / 10000.0f),
		m_fSpread, MAX_PARTICLES / 3);
	LoadParticles(&pData1[MAX_PARTICLES / 3],
		XMFLOAT3(-m_fSpread, 0, 0), XMFLOAT4(0, 0, 8, 1 / 10000.0f / 10000.0f),
		m_fSpread, MAX_PARTICLES / 2);
	LoadParticles(&pData1[2 * (MAX_PARTICLES / 3)],
		XMFLOAT3(0, 0, m_fSpread * 15.0f), XMFLOAT4(0, 0, -60, 1 / 10000.0f / 10000.0f),
		m_fSpread, MAX_PARTICLES - 2 * (MAX_PARTICLES / 3));
#endif

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = pData1;
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&desc,
			&InitData,
			m_pParticlePosVelo0.GetAddressOf()
			)
		);

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&desc,
			&InitData,
			m_pParticlePosVelo1.GetAddressOf()
			)
		);

	SAFE_DELETE_ARRAY(pData1);

	D3D11_SHADER_RESOURCE_VIEW_DESC DescRV = {};
	DescRV.Format = DXGI_FORMAT_UNKNOWN;
	DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	DescRV.Buffer.FirstElement = 0;
	DescRV.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;


	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateShaderResourceView(
			m_pParticlePosVelo0.Get(),
			&DescRV,
			m_pParticlePosVeloRV0.GetAddressOf()
			)
		);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateShaderResourceView(
			m_pParticlePosVelo1.Get(),
			&DescRV,
			m_pParticlePosVeloRV1.GetAddressOf()
			)
		);
	D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV = {};
	DescUAV.Format = DXGI_FORMAT_UNKNOWN;
	DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	DescUAV.Buffer.FirstElement = 0;
	DescUAV.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(
			m_pParticlePosVelo0.Get(),
			&DescUAV,
			m_pParticlePosVeloUAV0.GetAddressOf()
			)
		);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(
			m_pParticlePosVelo1.Get(),
			&DescUAV,
			m_pParticlePosVeloUAV1.GetAddressOf()
			)
		);
}


//--------------------------------------------------------------------------------------
bool Sample3DSceneRenderer::RenderParticles(ID3D11DeviceContext* pd3dImmediateContext, CXMMATRIX mView, CXMMATRIX mProj)
{
	ID3D11BlendState* pBlendState0 = nullptr;
	ID3D11DepthStencilState* pDepthStencilState0 = nullptr;
	UINT SampleMask0, StencilRef0;
	XMFLOAT4 BlendFactor0;
	pd3dImmediateContext->OMGetBlendState(&pBlendState0, &BlendFactor0.x, &SampleMask0);
	pd3dImmediateContext->OMGetDepthStencilState(&pDepthStencilState0, &StencilRef0);

	pd3dImmediateContext->VSSetShader(m_pRenderParticlesVS.Get(), nullptr, 0);
	pd3dImmediateContext->GSSetShader(m_pRenderParticlesGS.Get(), nullptr, 0);
	pd3dImmediateContext->PSSetShader(m_pRenderParticlesPS.Get(), nullptr, 0);

	pd3dImmediateContext->IASetInputLayout(m_pParticleVertexLayout.Get());

	// Set IA parameters
	ID3D11Buffer* pBuffers[1] = { m_pParticleBuffer.Get() };
	UINT stride[1] = { sizeof(PARTICLE_VERTEX) };
	UINT offset[1] = { 0 };
	pd3dImmediateContext->IASetVertexBuffers(0, 1, pBuffers, stride, offset);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	ID3D11ShaderResourceView* aRViews[1] = { m_pParticlePosVeloRV0.Get() };
	pd3dImmediateContext->VSSetShaderResources(0, 1, aRViews);

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	pd3dImmediateContext->Map(m_pcbGS.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	auto pCBGS = reinterpret_cast<CB_GS*>(MappedResource.pData);
	XMStoreFloat4x4(&pCBGS->m_WorldViewProj, XMMatrixMultiply(mView, mProj));
	XMStoreFloat4x4(&pCBGS->m_InvView, XMMatrixInverse(nullptr, mView));
	pd3dImmediateContext->Unmap(m_pcbGS.Get(), 0);
	pd3dImmediateContext->GSSetConstantBuffers(0, 1, m_pcbGS.GetAddressOf());

	pd3dImmediateContext->PSSetShaderResources(0, 1, m_pParticleTexRV.GetAddressOf());
	pd3dImmediateContext->PSSetSamplers(0, 1, m_pSampleStateLinear.GetAddressOf());

	float bf[] = { 0.f, 0.f, 0.f, 0.f };
	pd3dImmediateContext->OMSetBlendState(m_pBlendingStateParticle.Get(), bf, 0xFFFFFFFF);
	pd3dImmediateContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);

	pd3dImmediateContext->Draw(MAX_PARTICLES, 0);

	ID3D11ShaderResourceView* ppSRVNULL[1] = { nullptr };
	pd3dImmediateContext->VSSetShaderResources(0, 1, ppSRVNULL);
	pd3dImmediateContext->PSSetShaderResources(0, 1, ppSRVNULL);

	/*ID3D11Buffer* ppBufNULL[1] = { nullptr };
	pd3dImmediateContext->GSSetConstantBuffers( 0, 1, ppBufNULL );*/

	pd3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
	pd3dImmediateContext->OMSetBlendState(pBlendState0, &BlendFactor0.x, SampleMask0);
	SAFE_RELEASE(pBlendState0);
	pd3dImmediateContext->OMSetDepthStencilState(pDepthStencilState0, StencilRef0);
	SAFE_RELEASE(pDepthStencilState0);
	return true;
}


void Sample3DSceneRenderer::OnFrameMove()
{
	HRESULT hr;

	auto pd3dImmediateContext = m_deviceResources->GetD3DDeviceContext();

	int dimx = int(ceil(MAX_PARTICLES / 128.0f));

	{
		pd3dImmediateContext->CSSetShader(m_pCalcCS.Get(), nullptr, 0);

		// For CS input            
		ID3D11ShaderResourceView* aRViews[1] = { m_pParticlePosVeloRV0.Get() };
		pd3dImmediateContext->CSSetShaderResources(0, 1, aRViews);

		// For CS output
		ID3D11UnorderedAccessView* aUAViews[1] = { m_pParticlePosVeloUAV1.Get() };
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, (UINT*)(&aUAViews));

		// For CS constant buffer
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		DX::ThrowIfFailed(
			pd3dImmediateContext->Map(m_pcbCS.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)
			);
		auto pcbCS = reinterpret_cast<CB_CS*>(MappedResource.pData);
		pcbCS->param[0] = MAX_PARTICLES;
		pcbCS->param[1] = dimx;
		pcbCS->paramf[0] = 0.1f;
		pcbCS->paramf[1] = 1;
		pd3dImmediateContext->Unmap(m_pcbCS.Get(), 0);
		ID3D11Buffer* ppCB[1] = { m_pcbCS.Get() };
		pd3dImmediateContext->CSSetConstantBuffers(0, 1, ppCB);

		// Run the CS
		pd3dImmediateContext->Dispatch(dimx, 1, 1);

		// Unbind resources for CS
		ID3D11UnorderedAccessView* ppUAViewNULL[1] = { nullptr };
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, ppUAViewNULL, (UINT*)(&aUAViews));
		ID3D11ShaderResourceView* ppSRVNULL[1] = { nullptr };
		pd3dImmediateContext->CSSetShaderResources(0, 1, ppSRVNULL);

		//pd3dImmediateContext->CSSetShader( nullptr, nullptr, 0 );

		std::swap(m_pParticlePosVelo0, m_pParticlePosVelo1);
		std::swap(m_pParticlePosVeloRV0, m_pParticlePosVeloRV1);
		std::swap(m_pParticlePosVeloUAV0, m_pParticlePosVeloUAV1);
	}
}
