# SimpleRenderer
이런 저런 렌더링 공부용 렌더러

https://github.com/cakememakeme/ProjectCpuRenderer/ 에서 코드 이전

------------------
# CPU Renderer

![CPU 렌더러](https://github.com/cakememakeme/ProjectCpuRender/assets/73391410/52c763f8-e754-4c39-bde1-a86f3487d3c7)
라이트 컬러를 *직접* 조절했습니다. 

기본적인 라이트 구성이며, 위 GUI에 구성되어 있는 기능 이외의 추가적인 구현은 하지 않았습니다

대략적인 클래스 구성도는 다음과 같습니다
![클래스 다이어그램](https://github.com/cakememakeme/ProjectCpuRender/assets/73391410/ed219aa8-ef3d-436d-a30c-cc0d993fb919)
★ 주요 기능은 CpuRenderPipeline.h/cpp, CpuRasterizer.h/cpp 클래스에서 수행됩니다


★ 필요 프로그램&라이브러리
1. Visual Studio 2022

2. C++17

3. Direct3D 11

4. vcpkg설치
https://vcpkg.io/en/getting-started.html
https://github.com/microsoft/vcpkg

4.1

- vcpkg { Assimp, Imgui } 설치
      
4.1.a

- vcpkg install assimp:x64-windows

4.1.b

- vcpkg install imgui[win32-binding,dx11-binding]:x64-windows

- vcpkg integrate install



# 업데이트

(07/16) 클리핑 추가
![image](https://github.com/cakememakeme/ProjectCpuRender/assets/73391410/8144b321-9008-458c-b0bd-ad4c06117a33)

(02/19) CpuRenderer 코드 정리

(02/28) 근평면 클리핑 추가

------------------
# D3D11 Renderer

구현중
