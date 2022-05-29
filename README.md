#ESTAR HOMEWORK DOCUMENT
网易互娱E星计划--2组--杨奇璇

##1.直接运行
解压作业压缩包后，进入根目录下的MY_BGFX_LOVE文件夹中，双击MyRenderer.exe即可运行。

##2.编译和调试
在根目录下打开终端，依次运行：
```
mkdir build
cd build
cmake ../
```
然后打开build目录下的VS工程文件--EStarHomework.sln即可。  
resource/shaders文件夹会同时被复制到build中。
目前编译了OPENES和dx11两个平台的Shader，经测试都能够正常运行。

##3.shader介绍
shader的源码存放在resource目录下的shaders_Code文件夹中。
编译好的shader存放在resource目录下的shaders文件夹中。
每一个shader的文件夹中存放着顶点着色器，片元着色器以及定义变量的varying.def.sc。

* LightPoint 渲染点光源用，方便调试时查看点光源位置.  

* Diffuse 半兰伯特模型，用于前四个level中plane的渲染.  

* BlinnPhong 布林冯光照模型，环境光用常量控制.  

* BlinnPhongTex 布林冯光照模型，采样了法线和颜色纹理.  

* PBR 基于物理的材质.  

* SkyBox 采样第0级LOD环境光贴图，用于渲染天空盒.  

* IBL 通过IBL的方法计算环境光照，包括diffuse项和specular项.  

* Shadow 从光源计算shadowMap时采用的shader  

* ShadowMesh BlinnPhong的基础上加入了采样shadowmap计算得到的直接光visibility项。  

##4.代码的组织和参考
* Camera.h
主要负责处理鼠标和键盘的输入并控制相机，：  
	* 对鼠标输入的处理和控制参考了geometryv中相机的实现方式
	* 对键盘输入的处理和控制参考了examples/common中相机的实现方式
	* 相机的接口实现参考了examples/common中相机的实现方式，  
	维护了一个全局的camera在Camera.h中,并通过其中的函数进行相机状态的更新和相机参数的读取。

* homework.cpp  
这里按照从上往下的顺序依次介绍各部分
 	* Settings:   
 	定义了用于UI控制和传参的变量
 	* skyBoxState:   
 	定义了skybox和渲染状态（CW）
 	* LightProbe:   
 	参考了ibl-example, 方便切换和销毁CubeMap
 	* init():   
 	AppI中init的重写：设置了一些应用程序的基本参数;  
 	设置了各View的Clear方式;  
 	创建Uniforms;  
 	加载shader程序, 模型和纹理;  
 	设置了shadowMap用到的渲染状态;  
 	计算stone的AABB包围盒用于确定相机初始参数.
 	* shutdown():   
 	AppI中destroy的重写: 销毁实例.  
 	* bgfx::View的组织: 
 	用到了三个View:  
 	View0用于shadowmap的计算;  
 	View1用于Skybox的绘制;  
 	View2用于场景的渲染。
 	* update():   
 	AppI中update的重写:  编写和配置了IMGUI;  
 	配置了用于shadowmap的framebuffer和texture;  
 	设置了时间和一些基本的Uniforms;  
 	更新相机位置;   
 	计算了各个view的观察和投影矩阵;  
 	设置了各个模型的model矩阵;  
 	最后设置了纹理并提交了渲染资源,完成一帧的渲染。

##5.功能
参见演示视频
