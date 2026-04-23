Portfolio

包括项目效果以及源码、包括美工类作品的展示。
可查看视频的附属链接如下：https://www.feicut.com/fv/FV7e8rq6efr9?l=1

----------------------------------------

3D 软光栅化与光线追踪双引擎渲染器 - 主开发者 2025.09-- 2025.12

- 零API底层架构：脱离 OpenGL/Vulkan 等底层图形 API，仅使用 C++ 与 SDL3 独立构建了包含完整三维数学库的 3D 渲染管线，深刻验证了对现代 GPU 渲染底层逻辑的掌握。
- 核心光栅化管线：独立实现齐次空间下的近裁剪面几何分割；推导并实现了带透视校正（1/W）的重心坐标插值，彻底解决了纹理映射的仿射畸变问题。
- 高级光影与材质系统：实现了 Blinn-Phong 多光源光照模型与切线空间法线贴图；构建 Shadow Map 阴影映射，通过动态 DepthBias 与 3x3 PCF 滤波，成功解决了自阴影条纹与阴影悬浮的彼得潘现象。

以下是 visual studio 中运行的初次结果，移动、旋转以及亮度加减效果等详情见附属链接：
<br><img width="700" src="https://github.com/user-attachments/assets/ff4946fb-26da-4432-bb22-f852d69e0394" />

三个角度的渲染结果如下：
- view1: <br><img width="700" src="https://github.com/user-attachments/assets/9231d305-460e-4270-bb4c-99eabf271cd1" />
- view2: <br><img width="700" src="https://github.com/user-attachments/assets/af7b6989-c40d-4979-85a7-25ce075dd7af" />
- view3: <br><img width="700" src="https://github.com/user-attachments/assets/ce6fc63f-6467-4ada-8c5f-f538b0b70dbb" />

----------------------------------------

3D解密独立游戏项目《LOADIN》 - 独立开发者 2025.04 -- 2025.10

- 游戏系统设计与开发：独立完成游戏概念、玩法机制（解谜、探索）、关卡流程及用户界面（UI/UX）的规划与实现，构建了完整的游戏循环；搭建了健壮的游戏状态管理系统。
- 核心渲染特效：基于Unity URP，运用Shader Graph独立开发全屏扫描材质特效；通过后期处理体积实现动态镜头畸变、景深及色差调整。
- 交互与系统：使用C#实现扫描仪交互逻辑，驱动后期处理体积（PostProcessVolume），包括射线检测、物品高亮与信息UI显示。

主要效果如下：
<br><img width="700" src="https://github.com/user-attachments/assets/22de63cc-9801-48fe-bcd5-db7c32218e0e" />
<br><img width="700" src="https://github.com/user-attachments/assets/f23cb258-5991-4c48-aec0-4464382698ef" />

----------------------------------------

2D横版跳跃游戏 《I Wanna ！Learn ? 》 - 独立开发者 2024.11-- 2024.12

- 创新机制设计：独立设计并制作一款融合“I Wanna”高难度横版闯关与英语词汇学习的游戏，引入“答题复活”核心机制。
- 关卡与系统实现：独立完成多变关卡设计；实现角色控制、碰撞检测、答题判断、词库集成等核心玩法逻辑。
- 学习乐趣结合：有效将教育元素融入娱乐性横版游戏，提升学习趣味性及游戏策略性。

关卡效果如下：
<br><img width="700" src="https://github.com/user-attachments/assets/1059f30a-88ac-4305-9cf8-dba5e10f0851" />

----------------------------------------

游戏分析与策划研究

- 研究目的：对《空洞骑士：丝之歌》进行深度系统性分析，涵盖战斗系统、地图设计、关卡设计、叙事结构。
- 分析内容：拆解其银河战士恶魔城设计理念、Boss战机制、探索奖励循环、非线性叙事手法，并提出潜在优化与创新点。

----------------------------------------

剪辑与建模

- 剪辑视频详情见附属链接。
- 建模展示如下：
https://github.com/user-attachments/assets/f622516c-a219-4b07-9435-6c318aa11419
- UI设计详情见文件夹的jsd文件以及页面图pdf。
