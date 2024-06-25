#include<Windows.h>#include<cstdint>#include<string>#include<format>#include<d3d12.h>#include<dxgi1_6.h>#include<cassert>#include<dxgidebug.h>#include<dxcapi.h>#include<math.h>#include<d3d11.h>#pragma comment(lib,"d3d12.lib")#pragma comment(lib,"dxgi.lib")#pragma comment(lib,"dxguid.lib")#pragma comment(lib,"dxcompiler.lib")struct Vector4 {	float x;	float y;	float z;	float w;};//ウィンドウプロシージャLRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {	//メッセージに応じてゲーム固有の処理を行う	switch (msg) {		//ウィンドウが破壊された	case WM_DESTROY:		//OSに対して、アプリの終了を伝える		PostQuitMessage(0);		return 0;	}	//標準のメッセージ処理を行う	return DefWindowProc(hwnd, msg, wparam, lparam);}//出力ウィンドウに文字を出す関数void Log(const std::wstring& message) {	OutputDebugStringA(ConvertString(message).c_str());}void Log(const std::string& message) {	OutputDebugStringA(message.c_str());}std::wstring ConvertString(const std::string& str) {	if (str.empty()) {		return std::wstring();	}	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);	if (sizeNeeded == 0) {		return std::wstring();	}	std::wstring result(sizeNeeded, 0);	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);	return result;}std::string ConvertString(const std::wstring& str) {	if (str.empty()) {		return std::string();	}	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);	if (sizeNeeded == 0) {		return std::string();	}	std::string result(sizeNeeded, 0);	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);	return result;}IDxcBlob* CompileShader(	//CompilerするShaderファイルへのパス	const std::wstring& filePath,	//Compilerに使用するProfile	const wchar_t* profile,	//初期化で生成したものを３つ	IDxcUtils* dxcUtils,	IDxcCompiler3* dxcCompiler,	IDxcIncludeHandler* includeHandler) {	//これからシェーダーをコンパイルする旨をログに出す	Log(ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n", filePath, profile)));	//hlslファイルを読む	IDxcBlobEncoding* shaderSource = nullptr;	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);	//読めなかったら止める	assert(SUCCEEDED(hr));	//読み込んだファイルの内容を設定する	DxcBuffer shaderSourceBuffer = {};	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();	shaderSourceBuffer.Size = shaderSource->GetBufferSize();	shaderSourceBuffer.Encoding = DXC_CP_UTF8;//UTF8の文字コードであることを通知	//Compileする	LPCWSTR arguments[] = {		filePath.c_str(),//コンパイル対象のhlslファイル名		L"-E",L"main",//エントリーポイントの指定。基本的にmain以外にはしない		L"-T",profile,//ShaderProfileの設定		L"-Zi",L"-Qembed_debug",//デバッグ用の情報を埋め込む		L"-0d" ,       //最適化を外しておく		L"-Zpr",      //メモリレイアウトは行優先	};	//実際にShaderをコンパイルする	IDxcResult* shaderResult = nullptr;	hr = dxcCompiler->Compile(		&shaderSourceBuffer,          //読み込んだファイル		arguments,                    //コンパイルオプション		_countof(arguments),          //コンパイルオプションの数		includeHandler,               //includeが含まれた諸々		IID_PPV_ARGS(&shaderResult)   //コンパイル結果	);	//コンパイルエラーではなくdxcが起動できないなど致命的な状況	assert(SUCCEEDED(hr));	//警告・エラーが出てたらログに出して止める	IDxcBlobUtf8* shaderError = nullptr;	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {		Log(shaderError->GetStringPointer());		//警告・エラーダメ絶対		assert(false);	}	//コンパイル結果から実効用のバイナリ部分を取得	IDxcBlob* shaderBlob = nullptr;	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);	assert(SUCCEEDED(hr));	//成功したログをだす	Log(ConvertString(std::format(L"Compile Succeeded,path:{},profile:{}\n", filePath, profile)));	shaderSource->Release();	shaderResult->Release();	//もう使わないリソースを解放	shaderSource->Release();}//Windowsアプリでのエントリーポイント(main関数)int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {	//出力ウィンドウへの文字出力	OutputDebugStringA("Hello,DirectX!\n");	WNDCLASS wc{};	//ウィンドウプロシージャ	wc.lpfnWndProc = WindowProc;	//ウィンドウクラス名	wc.lpszClassName = L"CG2WindowClass";	//インスタンスハンドル	wc.hInstance = GetModuleHandle(nullptr);	//カーソル	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);	//ウィンドウクラス	RegisterClass(&wc);	//クライアント領域のサイズ	const int32_t kClientWidth = 1280;	const int32_t kClientHeight = 720;	//ウィンドウサイズを表す構造体にクライアント領域を入れる	RECT wrc = { 0,0,kClientWidth,kClientHeight };	//クライアント領域を元に実際のサイズにwrcを変更してもらう	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);	//ウィンドウの生成	HWND hwnd = CreateWindow(		wc.lpszClassName,      //利用するクラス名		L"CG2",                //タイトルバーの文字(何でもいい)		WS_OVERLAPPEDWINDOW,   //よく見るウィンドウスタイル		CW_USEDEFAULT,          //表示X座標(Windowsに任せる）		CW_USEDEFAULT,          //表示Y座標(WindowsOSに任せる)		wrc.right - wrc.left,    //ウィンドウ横幅		wrc.bottom - wrc.top,    //ウィンドウ縦幅		nullptr,               //親ウィンドウハンドル		nullptr,               //メニューハンドル		wc.hInstance,          //インスタンスハンドル		nullptr);            //オプション#ifdef _DEBUG	ID3D12Debug1* debugController = nullptr;	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {		//デバッグレイヤーを無効化する		debugController->EnableDebugLayer();		//さらにGPU側でもチェックを行うようにする		debugController->SetEnableGPUBasedValidation(TRUE);	}#endif 	//ウィンドウを表示する	ShowWindow(hwnd, SW_SHOW);	//DXGIファクトリーの生成	IDXGIFactory7* dxgiFactory = nullptr;	//HRESULTはWindowe系のエラーコードであり、	//関数が成功したかどうかをSUCCEEDEDマクロで判定できる	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));	//初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、	// どうにもできない場合が多いのでassertにしておく	assert(SUCCEEDED(hr));	//使用するアダプタ用の変数。最初にnullptrを入れておく	IDXGIAdapter4* useAdapter = nullptr;	//いい順にアダプタを頼む	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=		DXGI_ERROR_NOT_FOUND; ++i) {		//アダプターの情報を登録する		DXGI_ADAPTER_DESC3 adapterDesc{};		hr = useAdapter->GetDesc3(&adapterDesc);		assert(SUCCEEDED(hr)); //取得できないのは一大事		//ソフトウェアアダプタでなければ採用！		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {			//採用したアダプタの情報をログに出力。wstringの方なので注意			Log(std::format(L"Use Adapater:{}\n", adapterDesc.Description));			break;		}		useAdapter = nullptr; //ソフトウェアアダプタの場合は見なかったことにする	}	//適切なアダプタが見つかなかったので起動できない	assert(useAdapter != nullptr);	ID3D12Device* device = nullptr;	//機能レベルとログ出力用の文字列	D3D_FEATURE_LEVEL featureLevels[] = {		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0	};	const char* featureLevelString[] = { "12.2","12.1","12.0" };	//高い順に生成できるか試していく	for (size_t i = 0; i < _countof(featureLevels); ++i) {		//採用したアダプターでデバイスを生成		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));		//指定した機能レベルでデバイスが生成できたかを確認		if (SUCCEEDED(hr)) {			//生成できたのでログ出力を行ってループを抜ける			Log(std::format("FeatureLevel:{}\n", featureLevelString[i]));			break;		}	}	//デバイスの生成がうまくいかなかったので起動できない	assert(device != nullptr);	Log("Complete create D3D12Device!!!\n");//初期化完了のログをだす#ifdef _DEBUG	ID3D12InfoQueue* infoQueue = nullptr;	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {		//やばいエラー時に止まる		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);		//エラー時に止まる		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);		//警告時に止まる		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);		//抑制するメッセージのID		D3D12_MESSAGE_ID denyIds[] = {			//Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ				//https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE };		//抑制するレベル		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };		D3D12_INFO_QUEUE_FILTER filter{};		filter.DenyList.NumIDs = _countof(denyIds);		filter.DenyList.pIDList = denyIds;		filter.DenyList.NumSeverities = _countof(severities);		filter.DenyList.pSeverityList = severities;		//指定したメッセージの表示を抑制する		infoQueue->PushStorageFilter(&filter);		//解放		infoQueue->Release();#endif // DEBUG		//コマンドキューを生成する		ID3D12CommandQueue* commandQueue = nullptr;		D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};		hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));		//コマンドキューの生成がうまくいかなかったので起動できない		assert(SUCCEEDED(hr));		//コマンドアロケータを生成する		ID3D12CommandAllocator* commandAllocator = nullptr;		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));		//コマンドアロケータの生成がうまくいかなかったので起動できない		assert(SUCCEEDED(hr));		//コマンドリストを生成する		ID3D12GraphicsCommandList* commandList = nullptr;		hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));		//コマンドリストの生成がうまくいかなかったので起動できない		assert(SUCCEEDED(hr));		//スワップチェーンを生成する		IDXGISwapChain4* swapChain = nullptr;		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};		swapChainDesc.Width = kClientWidth; //画面の幅。ウィンドウのクライアント領域を同じものにしておく		swapChainDesc.Height = kClientHeight; //画面の高さ。ウィンドウのクライアント領域を同じものにしておく		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //色の形式		swapChainDesc.SampleDesc.Count = 1; //マルチサンプルしない		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //描画のターゲットとして利用する		swapChainDesc.BufferCount = 2; //ダブルバッファ		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //モニタにうつしたら、中身を破棄		//コマンドキュー、ウィンドウハンドル、設定を渡して生成する		hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));		assert(SUCCEEDED(hr));		//ディスクリプタヒープの生成		ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;		D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};		rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビュー用		rtvDescriptorHeapDesc.NumDescriptors = 2; //ダブルバッファ用に2つ。多くても別に構わない		hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));		//ディスクリプタヒープが作れなかったので起動できない		assert(SUCCEEDED(hr));		//SwapChainからResourceを引っ張ってくる		ID3D12Resource* swapChainResources[2] = { nullptr };		hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));		//うまく取得できなければ起動できない		assert(SUCCEEDED(hr));		hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));		assert(SUCCEEDED(hr));		// RTVの設定		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2dテクスチャとして書き込む		// ディスクリプタの先頭を取得する		D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();		// RTVを2つ作るのでディスクリプタを2つ用意		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] = {};		// まず1つ目を作る。1つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある		rtvHandles[0] = rtvStartHandle;		device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);		// 2つ目のディスクリプタハンドルを得る（自力で）		rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);		// 2つ目を作る		device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);		MSG msg{};		//FenceとEventを生成する		//初期値0でFenceを作る		ID3D12Fence* fence = nullptr;		uint64_t fenceValue = 0;		hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));		assert(SUCCEEDED(hr));		//FenceのSignalを待つためのイベントを作成する		HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);		assert(fenceEvent != nullptr);		MSG msg{};		//dxcCompilerを初期化		IDxcUtils* dxcUtils = nullptr;		IDxcCompiler3* dxcCompiler = nullptr;		hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));		assert(SUCCEEDED(hr));		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));		assert(SUCCEEDED(hr));		//現時点でincludはしないが、includeに対応するための設定を行っておく		IDxcIncludeHandler* includeHandler = nullptr;		hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);		assert(SUCCEEDED(hr));		//-------------------		//RootSignature作成		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};		descriptionRootSignature.Flags =			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;		//シリアライズしてバイナリにする		ID3DBlob* signatureBlob = nullptr;		ID3DBlob* errorBlob = nullptr;		hr = D3D12SerializeRootSignature(&descriptionRootSignature,			D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);		if (FAILED(hr)) {			Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));			assert(false);		}		//バイナリをもとに生成		ID3D12RootSignature* rootSignature = nullptr;		hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),			IID_PPV_ARGS(&rootSignature));		assert(SUCCEEDED(hr));		//InputLayout		D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};		inputElementDescs[0].SemanticName = "POSITION";		inputElementDescs[0].SemanticIndex = 0;		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;		inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};		inputLayoutDesc.pInputElementDescs = inputElementDescs;		inputLayoutDesc.NumElements = _countof(inputElementDescs);		//BlendStateの設定		D3D12_BLEND_DESC blendDesc{};		//すべての色要素を書き込む		blendDesc.RenderTarget[0].RenderTargetWriteMask =			D3D12_COLOR_WRITE_ENABLE_ALL;		//ResiterzerStateの設定		D3D12_RASTERIZER_DESC rasterizerDesc{};		//裏面(時計回り)を表示しない		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;		//3角形の仲を塗りつぶす		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;		//shaderをコンパイルする		IDxcBlob* vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);		assert(vertexShaderBlob != nullptr);		IDxcBlob* pixelShaderBlob = CompileShader(L"Object3D.PShlsl",			L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);		assert(pixelShaderBlob != nullptr);		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};		graphicsPipelineStateDesc.pRootSignature = rootSignature;//RootSignature		graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;//INputLayout		graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),		vertexShaderBlob->GetBufferSize() };		graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),		pixelShaderBlob->GetBufferSize() };		graphicsPipelineStateDesc.BlendState = blendDesc;		graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;		graphicsPipelineStateDesc.NumRenderTargets = 1;		graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;		graphicsPipelineStateDesc.PrimitiveTopologyType =			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;		graphicsPipelineStateDesc.SampleDesc.Count = 1;		graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;		ID3D12PipelineState* graphicsPipelineState = nullptr;		hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,			IID_PPV_ARGS(&graphicsPipelineState));		assert(SUCCEEDED(hr));		D3D12_HEAP_PROPERTIES uploadHeapProperties{};		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;		D3D12_RESOURCE_DESC vertexResourceDesc{};		vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		vertexResourceDesc.Width = sizeof(Vector4) * 3;		vertexResourceDesc.Height = 1;		vertexResourceDesc.DepthOrArraySize = 1;		vertexResourceDesc.MipLevels = 1;		vertexResourceDesc.SampleDesc.Count = 1;		vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		ID3D12Resource* vertexResource = nullptr;		hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,			&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,			IID_PPV_ARGS(&vertexResource));		assert(SUCCEEDED(hr));		D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};		vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();		vertexBufferView.SizeInBytes = sizeof(Vector4) * 3;		vertexBufferView.StrideInBytes = sizeof(Vector4);		Vector4* vertexDate = nullptr;		vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexDate));		vertexDate[0] = { -0.5f,-0.5f,0.0f,1.0f };		vertexDate[1] = { 0.0f,0.5f,0.0f,1.0f };		vertexDate[2] = { 0.5f,-0.5f,0.0f,1.0f };		D3D12_VIEWPORT viewport{};		viewport.Width = kClientWidth;		viewport.Height = kClientHeight;		viewport.TopLeftX = 0;		viewport.TopLeftY = 0;		viewport.MinDepth = 0.0f;		viewport.MaxDepth = 1.0f;		D3D12_RECT scissorRect{};		scissorRect.left = 0;		scissorRect.right = kClientWidth;		scissorRect.top = 0;		scissorRect.bottom = kClientHeight;				D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;		D3D12_ROOT_PARAMETER rootParameters[1] = {};		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;		//ウィンドウの×ボタンが押されるまでループ		while (msg.message != WM_QUIT) {			//Windowにメッセージが来てたら最優先で処理される			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {				TranslateMessage(&msg);				DispatchMessage(&msg);			}			else {				//ゲームの処理				// 				// これから書き込むバックバッファのインデックスを取得				UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();				//TranstionBarrierの設定				D3D12_RESOURCE_BARRIER barrier{};				//今回のバリアはTransition				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;				//Noneにしておく				barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;				//バリアを貼る対象のリソース。現在のバックバッファに対して行う				barrier.Transition.pResource = swapChainResources[backBufferIndex];				//遷移前(現在)のResourceState				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;				//遷移後のResourceState				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;				//TransitionBarrierを貼る				commandList->ResourceBarrier(1, &barrier);				commandList->RSSetViewports(1, &viewport);				commandList->RSSetScissorRects(1, &scissorRect);				commandList->SetGraphicsRootSignature(rootSignature);				commandList->SetPipelineState(graphicsPipelineState);				commandList->IASetVertexBuffers(0, 1, &vertexBufferView);				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);				commandList->DrawInstanced(3, 1, 0, 0);				//初期値0でFenceを作る				ID3D12Fence* fence = nullptr;				uint64_t fenceValue = 0;				hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));				assert(SUCCEEDED(hr));				//FenceのSignalを待つためのイベントを作成する				HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);				assert(fenceEvent != nullptr);				// コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること				hr = commandList->Close();				assert(SUCCEEDED(hr));				// GPUにコマンドリストの実行を行わせる				ID3D12CommandList* commandLists[] = { commandList };				commandQueue->ExecuteCommandLists(1, commandLists);				// GPUとOSに画面の交換を行うよう通知する				swapChain->Present(1, 0);				//Fenceの値を更新				fenceValue++;				//GPUがここまでたどり着いた時に、Fenceの値を指定した値に代入するようにSignalを送る				commandQueue->Signal(fence, fenceValue);				//Fenceの値が指定したSignal値にたどり着いているか確認する				// GetCompletedValueの初期値はFence作成時に渡した初期値				if (fence->GetCompletedValue() < fenceValue) {					//指定したSignalにたどりついていないので、たどり着くまで待つようにイベントを設定する					fence->SetEventOnCompletion(fenceValue, fenceEvent);					//イベント待つ					WaitForSingleObject(fenceEvent, INFINITE);				}			}			//出力ウィンドウへの文字出力			OutputDebugStringA("Hello,DirectX!\n");			CloseHandle(fenceEvent);			fence->Release();			rtvDescriptorHeap->Release();			swapChainResources[0]->Release();			swapChainResources[1]->Release();			swapChain->Release();			commandList->Release();			commandQueue->Release();			device->Release();			useAdapter->Release();			vertexResource->Release();			graphicsPipelineState->Release();			signatureBlob->Release();			if (errorBlob) {				errorBlob->Release();			}			rootSignature->Release();			pixelShaderBlob->Release();			vertexShaderBlob->Release();#ifdef _DEBUG			debugController;#endif			CloseWindow(hwnd);			//リソースリースチェック			IDXGIDebug1* debug;			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {				debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);				debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);				debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);				debug->Release();			}		}		return 0;	}