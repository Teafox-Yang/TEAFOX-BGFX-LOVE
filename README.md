# TEAFOX-BGFX-LOVE
渲染器小DEMO，directX11平台和OpenES平台，基于基于BGFX渲染引擎框架编写
### 1.直接运行(WINDOWS)
解压作业压缩包后，进入根目录下的MY_BGFX_LOVE文件夹中，双击MyRenderer.exe即可运行。

### 2.编译和调试
在根目录下打开终端，依次运行：
```
mkdir build
cd build
cmake ../
```
打开build目录下的VS工程文件--TEAFOX-BGFX-RENDERERsln即可。  
resource/shaders文件夹会同时被复制到build中。
目前编译了OPENES和dx11两个平台的Shader，经测试都能够正常运行。

### 3.功能
##### Level1
1. 加载模型，绘制在屏幕上。
2. 添加了环绕相机（Orbit Camera）：
	- 鼠标左键拖拽以旋转镜头
	- 鼠标滚轮缩放镜头
	- 键盘ADWS控制镜头移动  
##### Level2
1. 为模型添加基础纹理和法线。
2. 为模型添加基础光照（Blinn-Phong）  
##### Level3
1. 经典PBR的实现（非kulla-Conty）
    - 模型的金属度能正确影响漫反射光照
    - 模型的albedo使用纹理控制
    - 模型的金属度，粗糙度，环境光遮蔽通过纹理控制
##### Level4
1. 使用IBL为模型添加了环境光照的diffuse和specular部分
2. 添加了可以切换的天空盒  

##### Level5
1. 计算ShadowMAP，使用PCS为模型添加阴影  
2. 
### 3.shader介绍
* shader的源码存放在resource目录下的shaders_Code文件夹中。
* 编译好的shader存放在resource目录下的shaders文件夹中。
* 每一个shader的文件夹中存放着顶点着色器，片元着色器以及定义输入输出结构的varying.def.sc
    * LightPoint 渲染点光源用，方便调试时查看点光源位置.  
    * Diffuse 半兰伯特模型，用于前四个level中plane的渲染.  
    * BlinnPhong 布林冯光照模型，环境光用常量控制.  
    * BlinnPhongTex 布林冯光照模型，采样了法线和颜色纹理.  
    * PBR 基于物理的材质.  
    * SkyBox 采样第0级LOD环境光贴图，用于渲染天空盒.  
    * IBL 通过IBL的方法计算基于物理的环境光着色，包括diffuse项和specular项.  
    * Shadow 从光源计算shadowMap时采用的shader  
    * ShadowMesh BlinnPhong的基础上加入了采样shadowmap计算得到的直接光visibility项。  

### 4.代码的组织
* Camera.h
环绕相机的编写（对鼠标和键盘控制的响应）
* homework.cpp  
这里按照从上往下的顺序依次介绍各部分
 	* Settings:   
 	定义了用于UI控制和传参的变量
 	* skyBoxState:   
 	定义了skybox和渲染状态（CW）
 	* LightProbe:   
 	方便切换和销毁CubeMap
 	* init():   
 	AppI中init的重写：  
        * 设置了一些应用程序的基本参数;  
 	    * 设置了各View的Clear方式;  
 	    * 创建Uniforms;  
 	    * 加载shader程序, 模型和纹理;  
 	    * 设置了shadowMap的渲染状态;  
 	    * 计算stone的AABB包围盒用于确定相机位置和裁剪平面.
 	* shutdown():   
 	AppI中destroy的重写: 销毁实例，释放空间.  
 	* bgfx::View的组织: 
 	用到了三个View（类似于PASS，但有区别）:  
 	    * View0用于shadowmap的计算;  
 	    * View1用于Skybox的绘制;  
 	    * View2用于场景的渲染。
 	* update():   
 	AppI中update的重写:  
        * 编写和配置了IMGUI;  
 	    * 配置了用于shadowmap的framebuffer和texture;  
 	    * 设置了时间和一些基本的Uniforms;  
 	    * 更新相机位置;   
    	* 计算了各个view的观察和投影矩阵;  
 	    * 设置了各个模型的model矩阵;  
 	    * 设置了纹理并提交了渲染资源,完成渲染。

