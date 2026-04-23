#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX 
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <random>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <omp.h>
#include<cfloat>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" 
// ==========================================
// 1. 数学库
// ==========================================
const int WIDTH = 800;
const int HEIGHT = 600;
const int SHADOW_RES = 1024; // 阴影贴图分辨率
const float PI = 3.14159265f;

enum InteractionMode {
	MODE_CAMERA,
	MODE_OBJECT,
	MODE_LIGHT
};

struct Vec2 { float x, y; Vec2() :x(0), y(0) {} Vec2(float a, float b) :x(a), y(b) {} };
struct Vec3 {
	float x, y, z;
	Vec3() : x(0), y(0), z(0) {}
	Vec3(float a) : x(a), y(a), z(a) {}
	Vec3(float a, float b, float c) : x(a), y(b), z(c) {}
	Vec3 operator+(const Vec3& r) const { return Vec3(x + r.x, y + r.y, z + r.z); }
	Vec3 operator-(const Vec3& r) const { return Vec3(x - r.x, y - r.y, z - r.z); }
	Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
	Vec3 operator*(const Vec3& r) const { return Vec3(x * r.x, y * r.y, z * r.z); }
	float dot(const Vec3& r) const { return x * r.x + y * r.y + z * r.z; }
	Vec3 cross(const Vec3& r) const { return Vec3(y * r.z - z * r.y, z * r.x - x * r.z, x * r.y - y * r.x); }
	Vec3 normalize() const { float l = std::sqrt(x * x + y * y + z * z); return l > 0 ? *this * (1.0f / l) : *this; }
	float length() const { return std::sqrt(x * x + y * y + z * z); }
};
inline Vec3 operator*(float s, const Vec3& v) { return Vec3(v.x * s, v.y * s, v.z * s); }

struct Vec4 {
	float x, y, z, w;
	Vec4() : x(0), y(0), z(0), w(1) {}
	Vec4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d) {}
	Vec4(Vec3 v, float d) : x(v.x), y(v.y), z(v.z), w(d) {}
};

struct Mat4 {
	float m[4][4];
	Mat4() { for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) m[i][j] = (i == j ? 1.0f : 0.0f); }
	//单位矩阵
	static Mat4 identity() { return Mat4(); }
	//平移矩阵
	static Mat4 translate(Vec3 v) { Mat4 r; r.m[0][3] = v.x; r.m[1][3] = v.y; r.m[2][3] = v.z; return r; }
	//缩放矩阵
	static Mat4 scale(Vec3 v) { Mat4 r; r.m[0][0] = v.x; r.m[1][1] = v.y; r.m[2][2] = v.z; return r; }
	static Mat4 rotateY(float rad) {
		Mat4 r; float c = cos(rad), s = sin(rad);
		r.m[0][0] = c; r.m[0][2] = s; r.m[2][0] = -s; r.m[2][2] = c; return r;
	}
	static Mat4 perspective(float fov, float aspect, float n, float f) {
		Mat4 r; for (int i = 0; i < 4; i++)for (int j = 0; j < 4; j++)r.m[i][j] = 0;
		float th = tan(fov / 2); r.m[0][0] = 1 / (aspect * th); r.m[1][1] = 1 / th;
		r.m[2][2] = -(f + n) / (f - n); r.m[2][3] = -(2 * f * n) / (f - n); r.m[3][2] = -1;
		return r;
	}
	// 正交投影矩阵：用于生成 Shadow Map
	static Mat4 ortho(float l, float r, float b, float t, float n, float f) {
		Mat4 m; m.m[0][0] = 2 / (r - l); m.m[0][3] = -(r + l) / (r - l); m.m[1][1] = 2 / (t - b); m.m[1][3] = -(t + b) / (t - b);
		m.m[2][2] = -2 / (f - n); m.m[2][3] = -(f + n) / (f - n); return m;
	}
	static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
		Vec3 f = (center - eye).normalize(); Vec3 s = f.cross(up).normalize(); Vec3 u = s.cross(f);
		Mat4 r; r.m[0][0] = s.x; r.m[0][1] = s.y; r.m[0][2] = s.z; r.m[0][3] = -s.dot(eye);
		r.m[1][0] = u.x; r.m[1][1] = u.y; r.m[1][2] = u.z; r.m[1][3] = -u.dot(eye);
		r.m[2][0] = -f.x; r.m[2][1] = -f.y; r.m[2][2] = -f.z; r.m[2][3] = f.dot(eye); return r;
	}
	Vec4 operator*(const Vec4& v) const {
		Vec4 r(0, 0, 0, 0);
		r.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w;
		r.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w;
		r.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w;
		r.w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w;
		return r;
	}
	Mat4 operator*(const Mat4& r) const {
		Mat4 res; for (int i = 0; i < 4; i++)for (int j = 0; j < 4; j++) { res.m[i][j] = 0; for (int k = 0; k < 4; k++) res.m[i][j] += m[i][k] * r.m[k][j]; } return res;
	}
};

// ==========================================
// 2. 纹理系统
// ==========================================
struct Texture { int w, h; std::vector<Vec3> pixels; };
std::vector<Texture> g_textures;
//生成纹理
void generateTextures() {
	Texture tex0; tex0.w = 256; tex0.h = 256;
	tex0.pixels.resize(256 * 256, Vec3(0.9f, 0.9f, 0.9f));
	g_textures.push_back(tex0);

	Texture tex1; tex1.w = 256; tex1.h = 256;
	tex1.pixels.resize(256 * 256);
	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < 256; x++) {
			float noise = (rand() % 100) / 100.0f;//生成0-1之间的随机噪点
			float line = sin(y * 0.2f + noise * 1.5f) * 0.1f;//模拟木头纹路
			Vec3 woodColor = Vec3(0.75f, 0.6f, 0.4f) + Vec3(0.1f, 0.08f, 0.05f) * line + Vec3(noise * 0.05f);
			if (x % 32 == 0) woodColor = woodColor * 0.7f;
			tex1.pixels[y * 256 + x] = woodColor;//二维坐标转一维索引
		}
	}
	g_textures.push_back(tex1);

	Texture tex2; tex2.w = 128; tex2.h = 128;
	tex2.pixels.resize(128 * 128);
	for (auto& p : tex2.pixels) { float v = (rand() % 100) / 100.0f; p = Vec3(v, v, v); }
	g_textures.push_back(tex2);
}

int loadTextureFile(const std::string& filename) {
	int width, height, channels;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 3);
	if (!data) { std::cerr << "纹理加载失败: " << filename << std::endl; return -1; }
	Texture tex; tex.w = width; tex.h = height; tex.pixels.resize(width * height);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = (y * width + x) * 3;
			tex.pixels[y * width + x] = Vec3(data[idx] / 255.0f, data[idx + 1] / 255.0f, data[idx + 2] / 255.0f);
		}
	}
	stbi_image_free(data); g_textures.push_back(tex);
	std::cout << "[OK] Texture loaded: " << filename << " -> ID " << g_textures.size() - 1 << std::endl;
	return (int)g_textures.size() - 1;
}

Vec3 sampleTexture(int id, Vec2 uv) {
	if (id < 0 || id >= (int)g_textures.size()) return Vec3(1, 1, 1);
	const Texture& t = g_textures[id];

	// 将 UV 映射到像素坐标系（减去 0.5 是为了对齐像素中心）
	float u = uv.x * t.w - 0.5f;
	float v = uv.y * t.h - 0.5f;

	// 获取左上角的像素坐标
	int x0 = (int)std::floor(u);
	int y0 = (int)std::floor(v);
	int x1 = x0 + 1;
	int y1 = y0 + 1;

	// 计算当前点在四个像素之间的偏置比例 (权重)
	float fractX = u - std::floor(u);
	float fractY = v - std::floor(v);

	// 闭包函数：处理坐标越界（Repeat 模式）
	auto getPixel = [&](int x, int y) {
		x = (x % t.w + t.w) % t.w; // 保证负数也能正确取模
		y = (y % t.h + t.h) % t.h;
		return t.pixels[y * t.w + x];
		};

	// 拿到周围四个像素的颜色
	Vec3 c00 = getPixel(x0, y0);
	Vec3 c10 = getPixel(x1, y0);
	Vec3 c01 = getPixel(x0, y1);
	Vec3 c11 = getPixel(x1, y1);

	// X 方向两次插值
	Vec3 c0 = c00 * (1.0f - fractX) + c10 * fractX;
	Vec3 c1 = c01 * (1.0f - fractX) + c11 * fractX;

	// Y 方向最终插值
	return c0 * (1.0f - fractY) + c1 * fractY;
}

// ==========================================
// 3. 几何结构
// ==========================================
struct Vertex { Vec3 pos; Vec3 normal; Vec2 uv; };//定义顶点
struct Mesh {
	std::vector<Vertex> verts;
	std::vector<int> indices;
	Mat4 transform;
	Vec3 color;
	int texID;
	int normalMapID;
	int roughnessMapID;
	int metallicMapID;
	bool isTransparent;
	float alpha;
	float shininess;
	Vec3 posOffset;
	Vec3 scaleVec;
	Vec3 localOffset;
	// AABB 包围盒数据：用于光线追踪时的加速（先检测盒子，再检测内部三角形）
	Vec3 minLocalBounds; //局部坐标系下的最小值
	Vec3 maxLocalBounds; //局部坐标系下的最大值
	Vec3 minWorldBounds;
	Vec3 maxWorldBounds;
	bool castShadow = true; // 默认所有物体都产生阴影
};

// 根据当前的 transform 更新世界坐标 AABB
void updateMeshAABB(Mesh& m) {
	// 局部包围盒的 8 个角点
	Vec3 corners[8];
	corners[0] = Vec3(m.minLocalBounds.x, m.minLocalBounds.y, m.minLocalBounds.z);
	corners[1] = Vec3(m.maxLocalBounds.x, m.minLocalBounds.y, m.minLocalBounds.z);
	corners[2] = Vec3(m.minLocalBounds.x, m.maxLocalBounds.y, m.minLocalBounds.z);
	corners[3] = Vec3(m.maxLocalBounds.x, m.maxLocalBounds.y, m.minLocalBounds.z);
	corners[4] = Vec3(m.minLocalBounds.x, m.minLocalBounds.y, m.maxLocalBounds.z);
	corners[5] = Vec3(m.maxLocalBounds.x, m.minLocalBounds.y, m.maxLocalBounds.z);
	corners[6] = Vec3(m.minLocalBounds.x, m.maxLocalBounds.y, m.maxLocalBounds.z);
	corners[7] = Vec3(m.maxLocalBounds.x, m.maxLocalBounds.y, m.maxLocalBounds.z);
	//初始化世界包围盒为无效
	m.minWorldBounds = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	m.maxWorldBounds = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int i = 0; i < 8; i++) {
		// 将角点变换到世界坐标
		Vec4 worldPos4 = m.transform * Vec4(corners[i], 1.0f);
		Vec3 worldPos(worldPos4.x, worldPos4.y, worldPos4.z);
		//更新左下角
		if (worldPos.x < m.minWorldBounds.x) m.minWorldBounds.x = worldPos.x;
		if (worldPos.y < m.minWorldBounds.y) m.minWorldBounds.y = worldPos.y;
		if (worldPos.z < m.minWorldBounds.z) m.minWorldBounds.z = worldPos.z;
		//更新右上角
		if (worldPos.x > m.maxWorldBounds.x) m.maxWorldBounds.x = worldPos.x;
		if (worldPos.y > m.maxWorldBounds.y) m.maxWorldBounds.y = worldPos.y;
		if (worldPos.z > m.maxWorldBounds.z) m.maxWorldBounds.z = worldPos.z;
	}

	// 稍微扩大一点，防止平面物体厚度为0
	Vec3 epsilon(0.001f, 0.001f, 0.001f);
	m.minWorldBounds = m.minWorldBounds - epsilon;
	m.maxWorldBounds = m.maxWorldBounds + epsilon;
}

struct SceneObject {
	std::string name;
	std::vector<int> meshIndices;//全局mssh列表的索引
	Vec3 position;
	float rotation;

	void updateTransforms(std::vector<Mesh>& meshes, std::vector<Mesh>& transpMeshes) {
		//计算父级变换矩阵
		Mat4 groupTransform = Mat4::translate(position) * Mat4::rotateY(rotation);

		for (int idx : meshIndices) {
			//普通mesh
			if (idx >= 0) {
				Mesh& m = meshes[idx];
				m.transform = groupTransform * Mat4::translate(m.localOffset) * Mat4::scale(m.scaleVec);
				m.posOffset = position + m.localOffset;
				updateMeshAABB(m); 
			}
			//透明mesh
			else {
				int transpIdx = -1 - idx;
				if (transpIdx >= 0 && transpIdx < (int)transpMeshes.size()) {
					Mesh& m = transpMeshes[transpIdx];
					m.transform = groupTransform * Mat4::translate(m.localOffset) * Mat4::scale(m.scaleVec);
					m.posOffset = position + m.localOffset;
					updateMeshAABB(m);
				}
			}
		}
	}
	//检查mesh是不是属于该物体
	bool containsMesh(int meshIdx) const {
		for (int idx : meshIndices) {
			if (idx == meshIdx) return true;
		}
		return false;
	}
};

std::vector<SceneObject> g_sceneObjects;

Mesh createBox(Vec3 pos, Vec3 scale, int texID, Vec3 color, int mask = 63, bool transp = false, float alpha = 1.0f, float shininess = 32.0f, bool flipNormals = false) {
	Mesh m;
	//初始化
	m.posOffset = pos;
	m.scaleVec = scale;
	m.localOffset = pos;
	m.transform = Mat4::translate(pos) * Mat4::scale(scale);
	m.color = color; m.texID = texID;
	m.normalMapID = -1; m.roughnessMapID = -1; m.metallicMapID = -1;
	m.isTransparent = transp; m.alpha = alpha; m.shininess = shininess;
	//顶点数据
	float vData[] = {
		-0.5,-0.5,0.5, 0,0,1, 0,0,  0.5,-0.5,0.5, 0,0,1, 1,0,  0.5,0.5,0.5, 0,0,1, 1,1,  -0.5,0.5,0.5, 0,0,1, 0,1,
		-0.5,-0.5,-0.5, 0,0,-1, 1,0, 0.5,-0.5,-0.5, 0,0,-1, 0,0, 0.5,0.5,-0.5, 0,0,-1, 0,1, -0.5,0.5,-0.5, 0,0,-1, 1,1,
		-0.5,0.5,-0.5, 0,1,0, 0,1,  0.5,0.5,-0.5, 0,1,0, 1,1,  0.5,0.5,0.5, 0,1,0, 1,0,  -0.5,0.5,0.5, 0,1,0, 0,0,
		-0.5,-0.5,-0.5, 0,-1,0, 0,0, 0.5,-0.5,-0.5, 0,-1,0, 1,0, 0.5,-0.5,0.5, 0,-1,0, 1,1, -0.5,-0.5,0.5, 0,-1,0, 0,1,
		-0.5,-0.5,-0.5, -1,0,0, 0,0, -0.5,0.5,-0.5, -1,0,0, 1,0, -0.5,0.5,0.5, -1,0,0, 1,1, -0.5,-0.5,0.5, -1,0,0, 0,1,
		0.5,-0.5,-0.5, 1,0,0, 1,0,  0.5,0.5,-0.5, 1,0,0, 1,1,  0.5,0.5,0.5, 1,0,0, 0,1,  0.5,-0.5,0.5, 1,0,0, 0,0
	};
	//填入m.verts
	for (int i = 0; i < 24; i++) {
		Vec3 n = Vec3(vData[i * 8 + 3], vData[i * 8 + 4], vData[i * 8 + 5]);
		if (flipNormals) {
			n = n * -1.0f; // 法线反转，朝向内部
		}
		m.verts.push_back({
			Vec3(vData[i * 8], vData[i * 8 + 1], vData[i * 8 + 2]),
			n,
			Vec2(vData[i * 8 + 6], vData[i * 8 + 7])
			});
	}

	int bases[] = { 0, 4, 8, 12, 16, 20 };//面的起始顶点索引
	int bits[] = { 1, 2, 4, 8, 16, 32 };
	for (int f = 0; f < 6; f++) {
		if (mask & bits[f]) {
			int b = bases[f];
			if (flipNormals) {
				// 逆转绘制顺序 (比如 0,1,2 变成 0,2,1)
				m.indices.push_back(b); m.indices.push_back(b + 2); m.indices.push_back(b + 1);
				m.indices.push_back(b + 2); m.indices.push_back(b); m.indices.push_back(b + 3);
			}
			else {
				// 原来的正常顺序
				m.indices.push_back(b); m.indices.push_back(b + 1); m.indices.push_back(b + 2);
				m.indices.push_back(b + 2); m.indices.push_back(b + 3); m.indices.push_back(b);
			}
		}
	}
	// Box 的顶点是 -0.5 到 0.5，所以局部包围盒是固定的
	m.minLocalBounds = Vec3(-0.5f, -0.5f, -0.5f);
	m.maxLocalBounds = Vec3(0.5f, 0.5f, 0.5f);
	updateMeshAABB(m); // 更新世界包围盒
	return m;
}

Mesh createLampshade(Vec3 pos, float topW, float bottomW, float height, Vec3 color) {
	Mesh m;
	m.posOffset = pos;
	m.scaleVec = Vec3(1, 1, 1);
	m.localOffset = pos;
	m.transform = Mat4::translate(pos);
	m.color = color; m.texID = -1;
	m.normalMapID = -1; m.roughnessMapID = -1; m.metallicMapID = -1;
	m.isTransparent = false; m.alpha = 1.0f; m.shininess = 32.0f;

	float hw = bottomW / 2, tw = topW / 2, h = height;
	Vec3 v[] = { {-hw, 0, hw}, {hw, 0, hw}, {hw, 0, -hw}, {-hw, 0, -hw}, {-tw, h, tw}, {tw, h, tw}, {tw, h, -tw}, {-tw, h, -tw} };
	//近似计算四个侧面的法线
	Vec3 nF = Vec3(0, 0.5f, 1).normalize(), nR = Vec3(1, 0.5f, 0).normalize(), nB = Vec3(0, 0.5f, -1).normalize(), nL = Vec3(-1, 0.5f, 0).normalize();

	m.verts.push_back({ v[0], nF, {0,0} }); m.verts.push_back({ v[1], nF, {1,0} }); m.verts.push_back({ v[5], nF, {1,1} }); m.verts.push_back({ v[4], nF, {0,1} });
	m.verts.push_back({ v[1], nR, {0,0} }); m.verts.push_back({ v[2], nR, {1,0} }); m.verts.push_back({ v[6], nR, {1,1} }); m.verts.push_back({ v[5], nR, {0,1} });
	m.verts.push_back({ v[2], nB, {0,0} }); m.verts.push_back({ v[3], nB, {1,0} }); m.verts.push_back({ v[7], nB, {1,1} }); m.verts.push_back({ v[6], nB, {0,1} });
	m.verts.push_back({ v[3], nL, {0,0} }); m.verts.push_back({ v[0], nL, {1,0} }); m.verts.push_back({ v[4], nL, {1,1} }); m.verts.push_back({ v[7], nL, {0,1} });
	//生成索引
	for (int i = 0; i < 4; i++) { int b = i * 4; m.indices.push_back(b); m.indices.push_back(b + 1); m.indices.push_back(b + 2); m.indices.push_back(b + 2); m.indices.push_back(b + 3); m.indices.push_back(b); }

	m.minLocalBounds = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	m.maxLocalBounds = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const auto& v : m.verts) {
		if (v.pos.x < m.minLocalBounds.x) m.minLocalBounds.x = v.pos.x;
		if (v.pos.y < m.minLocalBounds.y) m.minLocalBounds.y = v.pos.y;
		if (v.pos.z < m.minLocalBounds.z) m.minLocalBounds.z = v.pos.z;
		if (v.pos.x > m.maxLocalBounds.x) m.maxLocalBounds.x = v.pos.x;
		if (v.pos.y > m.maxLocalBounds.y) m.maxLocalBounds.y = v.pos.y;
		if (v.pos.z > m.maxLocalBounds.z) m.maxLocalBounds.z = v.pos.z;
	}
	updateMeshAABB(m);
	return m;
}

// ==========================================
// 4. MTL 加载器
// ==========================================
struct Material {
	std::string name;
	Vec3 diffuse;//物体本身颜色
	int texID;
	int normalMapID;
	int roughnessMapID;
	int metallicMapID;
	float shininess;
	Material() : diffuse(1, 1, 1), texID(-1), normalMapID(-1), roughnessMapID(-1), metallicMapID(-1), shininess(32.0f) {}
};

std::map<std::string, Material> loadMTL(const std::string& filename, const std::string& basePath) {
	//存储解析出的材质
	std::map<std::string, Material> materials;
	std::ifstream file(filename);
	if (!file.is_open()) { std::cerr << "[WARN] MTL file not found: " << filename << std::endl; return materials; }
	std::cout << "[INFO] Loading MTL: " << filename << std::endl;
	//指向正在解析的材质
	Material* currentMat = nullptr;
	std::string line;

	while (std::getline(file, line)) {
		line.erase(0, line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);
		if (line.empty() || line[0] == '#') continue;
		//拆分成单词
		std::istringstream iss(line);
		std::string type; iss >> type;//读取材质
		//新材质定义
		if (type == "newmtl") {
			std::string matName; iss >> matName;
			materials[matName] = Material();
			materials[matName].name = matName;
			currentMat = &materials[matName];
			std::cout << "  [MAT] Material: " << matName << std::endl;
		}
		else if (currentMat) {
			//漫反射颜色
			if (type == "Kd") iss >> currentMat->diffuse.x >> currentMat->diffuse.y >> currentMat->diffuse.z;
			//光泽度
			else if (type == "Ns") iss >> currentMat->shininess;
			//漫反射贴图
			else if (type == "map_Kd") {
				std::string texFile; iss >> texFile;
				int texID = loadTextureFile(basePath + texFile);
				if (texID >= 0) { currentMat->texID = texID; std::cout << "    [TEX] Base texture: " << texFile << " -> ID " << texID << std::endl; }
			}
			//法线贴图
			else if (type == "map_Bump" || type == "bump" || type == "norm" || type == "map_normal") {
				std::string token, texFile;
				while (iss >> token) {
					//是参数
					if (token[0] == '-') {
						std::string paramValue;
						iss >> paramValue;
					}
					else {
						texFile = token;
					}
				}
				if (!texFile.empty()) {
					int texID = loadTextureFile(basePath + texFile);
					if (texID >= 0) { currentMat->normalMapID = texID; std::cout << "    [NRM] Normal map: " << texFile << " -> ID " << texID << std::endl; }
				}
			}
		}
	}
	file.close();
	std::cout << "[OK] MTL loaded, " << materials.size() << " materials\n" << std::endl;
	return materials;
}

// ==========================================
// 5. OBJ 加载器
// ==========================================
Mesh loadOBJ(const std::string& filename, Vec3 pos, Vec3 scale, Vec3 defaultColor = Vec3(1, 1, 1)) {
	Mesh m;
	m.posOffset = pos;
	m.scaleVec = scale;
	m.localOffset = pos;
	// 注意：这里计算了变换矩阵
	m.transform = Mat4::translate(pos) * Mat4::scale(scale);
	//材质初始化
	m.color = defaultColor; m.texID = -1;
	m.normalMapID = -1; m.roughnessMapID = -1; m.metallicMapID = -1;
	m.isTransparent = false; m.alpha = 1.0f; m.shininess = 32.0f;

	size_t lastSlash = filename.find_last_of("/\\");
	std::string basePath = (lastSlash != std::string::npos) ? filename.substr(0, lastSlash + 1) : "";
	//临时容器，存储数据
	std::vector<Vec3> positions, normals;
	std::vector<Vec2> texCoords;
	std::map<std::string, Material> materials;
	Material* currentMaterial = nullptr;

	std::ifstream file(filename);
	if (!file.is_open()) { std::cerr << "[ERROR] Cannot open OBJ file: " << filename << std::endl; return m; }
	std::cout << "\n[INFO] Loading OBJ: " << filename << std::endl;
	std::string line;

	// --- 解析第一遍：读取数据 ---
	while (std::getline(file, line)) {
		line.erase(0, line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);
		if (line.empty() || line[0] == '#') continue;

		std::istringstream iss(line);
		std::string type; iss >> type;

		if (type == "mtllib") {
			std::string mtlFile; iss >> mtlFile;
			materials = loadMTL(basePath + mtlFile, basePath);
		}
		//解析顶点位置
		else if (type == "v") { float x, y, z; iss >> x >> y >> z; positions.push_back(Vec3(x, y, z)); }
		//法线向量
		else if (type == "vn") { float x, y, z; iss >> x >> y >> z; normals.push_back(Vec3(x, y, z).normalize()); }
		//纹理坐标
		else if (type == "vt") { float u, v; iss >> u >> v; texCoords.push_back(Vec2(u, 1.0f - v)); }
	}
	std::cout << "顶点统计: 位置=" << positions.size() << " 法线=" << normals.size() << " UV=" << texCoords.size() << std::endl;

	// --- 解析第二遍：构建面 ---
	file.clear(); file.seekg(0);//清除标志，移回开头

	while (std::getline(file, line)) {
		line.erase(0, line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);
		if (line.empty() || line[0] == '#') continue;
		std::istringstream iss(line);
		std::string type; iss >> type;
		//材质
		if (type == "usemtl") {
			std::string matName; iss >> matName;
			if (materials.find(matName) != materials.end()) {
				currentMaterial = &materials[matName];
				m.texID = currentMaterial->texID;
				m.normalMapID = currentMaterial->normalMapID;
				m.color = currentMaterial->diffuse;
				m.shininess = currentMaterial->shininess;
			}
		}
		//解析面
		else if (type == "f") {
			std::vector<int> vertIndices, texIndices, normalIndices;
			std::string vertex;

			while (iss >> vertex) {
				int vi = -1, ti = -1, ni = -1;
				size_t slash1 = vertex.find('/');
				if (slash1 == std::string::npos) vi = std::stoi(vertex) - 1;
				else {
					vi = std::stoi(vertex.substr(0, slash1)) - 1;
					size_t slash2 = vertex.find('/', slash1 + 1);
					if (slash2 == std::string::npos) { if (slash1 + 1 < vertex.length()) ti = std::stoi(vertex.substr(slash1 + 1)) - 1; }
					else {
						if (slash2 > slash1 + 1) ti = std::stoi(vertex.substr(slash1 + 1, slash2 - slash1 - 1)) - 1;
						if (slash2 + 1 < vertex.length()) ni = std::stoi(vertex.substr(slash2 + 1)) - 1;
					}
				}
				vertIndices.push_back(vi); texIndices.push_back(ti); normalIndices.push_back(ni);
			}
			//扇形拆分为三角形
			for (size_t i = 1; i < vertIndices.size() - 1; i++) {
				for (int j : {0, (int)i, (int)i + 1}) {
					Vertex v;
					if (vertIndices[j] >= 0 && vertIndices[j] < (int)positions.size()) v.pos = positions[vertIndices[j]];
					if (texIndices[j] >= 0 && texIndices[j] < (int)texCoords.size()) v.uv = texCoords[texIndices[j]]; else v.uv = Vec2(0, 0);
					if (normalIndices[j] >= 0 && normalIndices[j] < (int)normals.size()) v.normal = normals[normalIndices[j]]; else v.normal = Vec3(0, 1, 0);
					m.verts.push_back(v);
					m.indices.push_back((int)m.indices.size());
				}
			}
		}
	}
	file.close();

	// =========================================================
	// 计算世界坐标系下的 AABB 包围盒
	// =========================================================
	m.minLocalBounds = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	m.maxLocalBounds = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// 遍历所有顶点，只看局部坐标 (v.pos)
	for (const auto& v : m.verts) {
		if (v.pos.x < m.minLocalBounds.x) m.minLocalBounds.x = v.pos.x;
		if (v.pos.y < m.minLocalBounds.y) m.minLocalBounds.y = v.pos.y;
		if (v.pos.z < m.minLocalBounds.z) m.minLocalBounds.z = v.pos.z;

		if (v.pos.x > m.maxLocalBounds.x) m.maxLocalBounds.x = v.pos.x;
		if (v.pos.y > m.maxLocalBounds.y) m.maxLocalBounds.y = v.pos.y;
		if (v.pos.z > m.maxLocalBounds.z) m.maxLocalBounds.z = v.pos.z;
	}

	// 立即更新一次世界包围盒
	updateMeshAABB(m);
	// =========================================================

	std::cout << "[OK] OBJ loaded: verts=" << m.verts.size() << " tris=" << m.indices.size() / 3 << " texID=" << m.texID << std::endl << std::endl;
	return m;
}

std::vector<Mesh> g_objects;
std::vector<Mesh> g_transp_objects;

// ==========================================
// 6. 多光源渲染管线
// ==========================================
enum LightType { LIGHT_DIRECTIONAL, LIGHT_POINT, LIGHT_SPOT };

struct Light {
	LightType type;
	Vec3 pos, dir, color;
	float intensity;
	// 衰减系数 (Attenuation Factors): 决定了点光源/聚光灯随距离变暗的速度
	float constant = 1.0f, linear = 0.09f, quadratic = 0.032f;
	// 聚光灯的切光角 (CutOff): 决定了光锥的宽度
	float cutOff = cos(12.5f * PI / 180.0f), outerCutOff = cos(17.5f * PI / 180.0f);
};

std::vector<Light> g_lights;

struct VSOut { Vec4 pos; Vec3 worldPos; Vec3 normal; Vec2 uv; float w; };

std::vector<float> g_zBuffer(WIDTH* HEIGHT);
std::vector<uint32_t> g_frameBuffer(WIDTH* HEIGHT);
std::vector<float> g_shadowMap(SHADOW_RES* SHADOW_RES);

// 边函数：用于光栅化判断点是否在三角形内，以及计算重心坐标权重
float edgeFunction(const Vec2& a, const Vec2& b, const Vec2& c) { return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x); }

// ------------------------------------------
// 顶点着色器 (Main Vertex Shader)
// ------------------------------------------
// 作用：将模型空间的顶点变换到裁剪空间，并输出世界坐标、法线等给像素着色器插值使用
VSOut mainVS(const Vertex& v, const Mat4& model, const Mat4& viewProj) {
	VSOut out;
	// 1. 计算世界坐标：Model Matrix * LocalPos
	Vec4 wp = model * Vec4(v.pos, 1.0f); out.worldPos = Vec3(wp.x, wp.y, wp.z);

	// 2. 计算法线矩阵 (Normal Matrix)：为了处理非均匀缩放对法线的破坏
	float sx = Vec3(model.m[0][0], model.m[0][1], model.m[0][2]).length();
	float sy = Vec3(model.m[1][0], model.m[1][1], model.m[1][2]).length();
	float sz = Vec3(model.m[2][0], model.m[2][1], model.m[2][2]).length();
	Vec3 invScale(1.0f / sx, 1.0f / sy, 1.0f / sz);

	// 输出法线：在世界空间中用于光照计算
	Vec4 nRaw = model * Vec4(v.normal * invScale, 0.0f);
	out.normal = Vec3(nRaw.x, nRaw.y, nRaw.z).normalize();

	// 3. 计算裁剪空间坐标：ViewProjection * WorldPos -> 决定像素在屏幕的位置
	out.pos = viewProj * wp; out.uv = v.uv;
	return out;
}

// 阴影顶点着色器
Vec4 shadowVS(const Vertex& v, const Mat4& model, const Mat4& lp) { return lp * model * Vec4(v.pos, 1.0f); }

// ------------------------------------------
// 光照计算核心 (Blinn-Phong)
// ------------------------------------------
// 计算单个光源对某个着色点的漫反射和高光贡献
Vec3 CalcLight(Light light, Vec3 normal, Vec3 viewDir, Vec3 fragPos, float shininess, float shadowFactor) {
	Vec3 lightDir; float attenuation = 1.0f;

	// 1. 计算光照方向向量 L
	if (light.type == LIGHT_DIRECTIONAL) lightDir = (Vec3(0, 0, 0) - light.dir).normalize();
	else {
		lightDir = (light.pos - fragPos).normalize();
		float distance = (light.pos - fragPos).length();
		attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
	}

	// 2. 聚光灯边缘软化
	if (light.type == LIGHT_SPOT) {
		float theta = lightDir.dot((Vec3(0, 0, 0) - light.dir).normalize());
		float epsilon = light.cutOff - light.outerCutOff;
		float intensityFactor = std::max(0.0f, std::min(1.0f, (theta - light.outerCutOff) / epsilon));
		attenuation *= intensityFactor;
	}

	// 3. 漫反射
	float diff = std::max(normal.dot(lightDir), 0.0f);

	// 4. 高光
	Vec3 halfwayDir = (lightDir + viewDir).normalize();
	float spec = std::pow(std::max(normal.dot(halfwayDir), 0.0f), shininess);

	// 5. 组合结果
	Vec3 diffuse = light.color * diff * light.intensity * attenuation;
	Vec3 specular = light.color * spec * light.intensity * attenuation;

	// 6. 应用阴影遮挡
	if (light.type == LIGHT_DIRECTIONAL) {
		diffuse = diffuse * (1.0f - shadowFactor);
		specular = specular * (1.0f - shadowFactor);
	}
	return diffuse + specular;
}

Vec3 pixelShader(const VSOut& in, const Mesh& mesh, const std::vector<Light>& lights, const Vec3& camPos, const Mat4& ls, bool isSelected) {

	// --- 特殊处理：如果纹理 ID 为 2（可能是特定发光物），直接返回双倍亮度的采样色 ---
	if (mesh.texID == 2) return sampleTexture(mesh.texID, in.uv) * 2.0f;

	// --- 1. 基础纹理颜色获取 ---
	// 逻辑：如果网格有纹理，则采样并乘以材质本身的颜色；否则直接使用材质色
	Vec3 texColor = (mesh.texID >= 0) ? sampleTexture(mesh.texID, in.uv) * mesh.color : mesh.color;

	// --- 2. 选中高亮动画 ---
	// 逻辑：使用 SDL_GetTicks() 配合正弦函数产生 0.0 到 0.6 之间的呼吸频率，叠加到基础色上
	if (isSelected) {
		float pulse = (sin(SDL_GetTicks() * 0.005f) + 1.0f) * 0.3f;
		texColor = texColor + Vec3(pulse, pulse * 0.5f, 0);
	}

	// --- 3. 阴影计算 ---
	float shadow = 0.0f;
	// 将世界空间坐标变换到光源空间 
	Vec4 flp = ls * Vec4(in.worldPos, 1.0f);
	// 透视除法，获取 NDC 坐标 
	Vec3 proj = Vec3(flp.x, flp.y, flp.z) * (1.0f / flp.w);
	// 将坐标映射从 [-1, 1] 变换到 [0, 1]
	proj = proj * 0.5f + Vec3(0.5f);

	// 检查该点是否落在光源的投影范围内
	if (proj.z < 1 && proj.x > 0 && proj.x < 1 && proj.y > 0 && proj.y < 1) {
		// 这能有效解决“阴影痤疮 (Shadow Acne)”现象，即物体表面错误的自阴影条纹
		Vec3 lightDir = (Vec3(0, 0, 0) - lights[0].dir).normalize();
		float bias = std::max(0.006f * (1.0f - in.normal.dot(lightDir)), 0.00005f);

		// PCF 阴影平滑
		for (int x = -1; x <= 1; ++x) {
			for (int y = -1; y <= 1; ++y) {
				// 计算采样坐标
				int row = int(std::min(std::max((1.0f - proj.y) * SHADOW_RES + y, 0.f), (float)SHADOW_RES - 1));
				int col = int(std::min(std::max(proj.x * SHADOW_RES + x, 0.f), (float)SHADOW_RES - 1));
				float pcf = g_shadowMap[row * SHADOW_RES + col];

				// 如果当前点深度投影大于 ShadowMap 存储的深度，则认为在阴影中
				if ((proj.z - bias) > pcf) shadow += 1.0f;
			}
		}
		shadow /= 9.0f; // 取 9 个样本的平均值，产生软阴影边缘
	}

	// --- 4. 法线贴图处理 (Normal Mapping) ---
	Vec3 N = in.normal.normalize(); // 默认使用插值后的几何法线
	if (mesh.normalMapID >= 0) {
		Vec3 normalMapColor = sampleTexture(mesh.normalMapID, in.uv);
		if (normalMapColor.length() > 0.1f) {
			// 将 RGB 颜色 [0, 1] 转换回法线向量范围 [-1, 1]
			Vec3 tangentNormal = normalMapColor * 2.0f - Vec3(1.0f, 1.0f, 1.0f);
			tangentNormal = tangentNormal.normalize();

			// 切线空间构造
			Vec3 T = (std::abs(N.x) > 0.9f) ? Vec3(0.0f, 1.0f, 0.0f) : Vec3(1.0f, 0.0f, 0.0f);
			T = (T - N * N.dot(T)).normalize(); // 施密特正交化
			Vec3 B = N.cross(T).normalize();

			// 将法线从切线空间转换到世界空间
			N = (T * tangentNormal.x + B * tangentNormal.y + N * tangentNormal.z).normalize();
		}
	}

	// --- 5. 光照计算  ---
	Vec3 V = (camPos - in.worldPos).normalize(); // 视线向量
	Vec3 result(0, 0, 0);
	// 累加所有光源对该像素的颜色贡献
	for (const auto& l : lights) {
		result = result + CalcLight(l, N, V, in.worldPos, mesh.shininess, shadow);
	}

	// --- 6. 环境光 ---
	// 基础环境光 + 一个简单的方向性环境光（模拟来自天空的微弱散射）
	float amb = 0.35f + std::max(N.dot(Vec3(0, 1, 0)), 0.0f) * 0.15f;
	result = result * texColor + texColor * amb;

	// --- 7. 透明度效果 (菲涅尔近似) ---
	// 逻辑：当视角与表面法线夹角很大（擦边看）时，增强边缘高光，模拟水或玻璃的边缘反射
	if (mesh.isTransparent) {
		float f = pow(1.0f - std::max(0.0f, N.dot(V)), 3.0f) * 0.4f; // 菲涅尔系数
		result = result * (1.0f - f) + Vec3(0.8f, 0.9f, 1.0f) * f;
	}

	// --- 8. 色调映射  ---
	// 使用 x / (1 + x) 算法将高动态范围 (HDR) 的颜色映射到 [0, 1] 范围
	return Vec3(result.x / (1 + result.x), result.y / (1 + result.y), result.z / (1 + result.z));
}

// ------------------------------------------
// 阴影光栅化器
// ------------------------------------------
// 遍历三角形覆盖的像素，只记录深度值
void drawShadow(VSOut v0, VSOut v1, VSOut v2) {
	if (v0.pos.z < -v0.pos.w || v1.pos.z < -v1.pos.w || v2.pos.z < -v2.pos.w) return;
	auto toScreen = [&](VSOut& v) {
		float iw = 1.0f / v.pos.w;
		v.pos.x = (v.pos.x * iw + 1.0f) * 0.5f * SHADOW_RES;
		v.pos.y = (1.0f - (v.pos.y * iw + 1.0f) * 0.5f) * SHADOW_RES;
		v.pos.z = v.pos.z * iw * 0.5f + 0.5f;
		v.pos.w = iw;
		};
	toScreen(v0); toScreen(v1); toScreen(v2);
	int minX = std::max(0, (int)std::floor(std::min({ v0.pos.x, v1.pos.x, v2.pos.x })));
	int minY = std::max(0, (int)std::floor(std::min({ v0.pos.y, v1.pos.y, v2.pos.y })));
	int maxX = std::min(SHADOW_RES - 1, (int)std::ceil(std::max({ v0.pos.x, v1.pos.x, v2.pos.x })));
	int maxY = std::min(SHADOW_RES - 1, (int)std::ceil(std::max({ v0.pos.y, v1.pos.y, v2.pos.y })));
	Vec2 p0(v0.pos.x, v0.pos.y), p1(v1.pos.x, v1.pos.y), p2(v2.pos.x, v2.pos.y);
	float area = edgeFunction(p0, p1, p2);
	if (std::abs(area) < 1e-5) return;
	for (int y = minY; y <= maxY; y++) for (int x = minX; x <= maxX; x++) {
		Vec2 p(x + 0.5f, y + 0.5f);
		float w0 = edgeFunction(p1, p2, p) / area, w1 = edgeFunction(p2, p0, p) / area, w2 = edgeFunction(p0, p1, p) / area;
		if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
			float z = w0 * v0.pos.z + w1 * v1.pos.z + w2 * v2.pos.z;
			int idx = y * SHADOW_RES + x;
			if (z < g_shadowMap[idx]) g_shadowMap[idx] = z; // 深度测试：只记录最近的距离
		}
	}
}

// ------------------------------------------
// 主光栅化器 (Main Rasterizer)
// ------------------------------------------
// 辅助函数：根据比例 t 线性插值两个顶点的一切属性
VSOut lerpVertex(const VSOut& v1, const VSOut& v2, float t) {
	VSOut res;

	// Vec4 pos 手动插值
	res.pos.x = v1.pos.x + (v2.pos.x - v1.pos.x) * t;
	res.pos.y = v1.pos.y + (v2.pos.y - v1.pos.y) * t;
	res.pos.z = v1.pos.z + (v2.pos.z - v1.pos.z) * t;
	res.pos.w = v1.pos.w + (v2.pos.w - v1.pos.w) * t;

	// Vec3 worldPos 和 normal 你原来写了重载，可以直接用
	res.worldPos = v1.worldPos + (v2.worldPos - v1.worldPos) * t;
	res.normal = (v1.normal + (v2.normal - v1.normal) * t).normalize();

	// Vec2 uv 手动插值
	res.uv.x = v1.uv.x + (v2.uv.x - v1.uv.x) * t;
	res.uv.y = v1.uv.y + (v2.uv.y - v1.uv.y) * t;

	res.w = 0; // w占位，实际光栅化再处理
	return res;
}

// 核心裁剪逻辑：把与近裁剪面相交的三角形切成新的三角形
std::vector<VSOut> clipTriangleAgainstNearPlane(VSOut v0, VSOut v1, VSOut v2) {
	std::vector<VSOut> inside, outside, result;

	// 我们设定 w = 0.1 为近裁剪面
	(v0.pos.w >= 0.1f) ? inside.push_back(v0) : outside.push_back(v0);
	(v1.pos.w >= 0.1f) ? inside.push_back(v1) : outside.push_back(v1);
	(v2.pos.w >= 0.1f) ? inside.push_back(v2) : outside.push_back(v2);

	if (inside.size() == 3) {
		// 全在前面，不用切
		result = { v0, v1, v2 };
	}
	else if (inside.size() == 1 && outside.size() == 2) {
		// 1个点在前面，切成 1个 新的小三角形
		float t1 = (0.1f - inside[0].pos.w) / (outside[0].pos.w - inside[0].pos.w);
		float t2 = (0.1f - inside[0].pos.w) / (outside[1].pos.w - inside[0].pos.w);
		result.push_back(inside[0]);
		result.push_back(lerpVertex(inside[0], outside[0], t1));
		result.push_back(lerpVertex(inside[0], outside[1], t2));
	}
	else if (inside.size() == 2 && outside.size() == 1) {
		// 2个点在前面，切成 1个 四边形 (也就是 2个 三角形)
		float t1 = (0.1f - inside[0].pos.w) / (outside[0].pos.w - inside[0].pos.w);
		float t2 = (0.1f - inside[1].pos.w) / (outside[0].pos.w - inside[1].pos.w);
		VSOut newV1 = lerpVertex(inside[0], outside[0], t1);
		VSOut newV2 = lerpVertex(inside[1], outside[0], t2);

		result.push_back(inside[0]);
		result.push_back(inside[1]);
		result.push_back(newV1);

		result.push_back(inside[1]);
		result.push_back(newV2);
		result.push_back(newV1);
	}
	return result; // 返回0说明全在背后，被剔除了
}

// 遍历屏幕像素，计算重心插值，执行深度测试，调用 PixelShader 计算颜色，写入帧缓存
void drawTriangleMain(VSOut v0, VSOut v1, VSOut v2, const Mesh& mesh, const std::vector<Light>& lights, const Vec3& camPos, const Mat4& lMat, bool isSelected) {

	// lambda: 将裁剪空间坐标转换到屏幕空间，并预计算 1/W 用于透视校正
	auto toScreen = [&](VSOut& v) {
		float iw = 1.0f / v.pos.w; v.w = v.pos.w;
		v.pos.x = (v.pos.x * iw + 1.0f) * 0.5f * WIDTH;
		v.pos.y = (1.0f - (v.pos.y * iw + 1.0f) * 0.5f) * HEIGHT;
		// 所有的属性（Z, UV）都除以 W，以便在屏幕空间线性插值
		v.pos.z *= iw; v.uv.x *= iw; v.uv.y *= iw; v.pos.w = iw;

		v.worldPos = v.worldPos * iw;
		v.normal = v.normal * iw;

		v.pos.w = iw;
		};
	toScreen(v0); toScreen(v1); toScreen(v2);

	// 计算三角形的包围盒以减少遍历范围
	int minX = std::max(0, (int)std::floor(std::min({ v0.pos.x, v1.pos.x, v2.pos.x })));
	int minY = std::max(0, (int)std::floor(std::min({ v0.pos.y, v1.pos.y, v2.pos.y })));
	int maxX = std::min(WIDTH - 1, (int)std::ceil(std::max({ v0.pos.x, v1.pos.x, v2.pos.x })));
	int maxY = std::min(HEIGHT - 1, (int)std::ceil(std::max({ v0.pos.y, v1.pos.y, v2.pos.y })));
	Vec2 p0(v0.pos.x, v0.pos.y), p1(v1.pos.x, v1.pos.y), p2(v2.pos.x, v2.pos.y);
	float area = edgeFunction(p0, p1, p2);
	if (std::abs(area) < 1e-5) return;

#pragma omp parallel for
	for (int y = minY; y <= maxY; y++) for (int x = minX; x <= maxX; x++) {
		Vec2 p(x + 0.5f, y + 0.5f);
		// 计算重心坐标权重
		float w0 = edgeFunction(p1, p2, p) / area, w1 = edgeFunction(p2, p0, p) / area, w2 = edgeFunction(p0, p1, p) / area;
		if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
			// 插值 Z 值用于深度测试
			float z = w0 * v0.pos.z + w1 * v1.pos.z + w2 * v2.pos.z;
			int idx = y * WIDTH + x;

			// 深度测试
			if (z < g_zBuffer[idx]) {
				// 透视校正插值核心 
				// 先算出插值后的 1/W，取倒数得到当前的 W
				float oneOverW = w0 * v0.pos.w + w1 * v1.pos.w + w2 * v2.pos.w; float w = 1.0f / oneOverW;

				VSOut it;
				// 属性插值：(Attr/W * weight) * W_current
				it.worldPos = (w0 * v0.worldPos + w1 * v1.worldPos + w2 * v2.worldPos) * w;
				Vec3 interpNormal = (w0 * v0.normal + w1 * v1.normal + w2 * v2.normal) * w;
				it.normal = interpNormal.normalize();
				it.uv.x = (w0 * v0.uv.x + w1 * v1.uv.x + w2 * v2.uv.x) * w;
				it.uv.y = (w0 * v0.uv.y + w1 * v1.uv.y + w2 * v2.uv.y) * w;

				// 调用像素着色器计算颜色
				Vec3 col = pixelShader(it, mesh, lights, camPos, lMat, isSelected);
				uint8_t r = (uint8_t)std::min(255.0f, col.x * 255);
				uint8_t g = (uint8_t)std::min(255.0f, col.y * 255);
				uint8_t b = (uint8_t)std::min(255.0f, col.z * 255);

				// 写入 FrameBuffer 处理透明混合
				if (mesh.isTransparent) {
					uint32_t bg = g_frameBuffer[idx];
					uint8_t br = (bg >> 16) & 0xFF, bg_ = (bg >> 8) & 0xFF, bb = bg & 0xFF;
					float a = mesh.alpha;
					// Alpha 混合：Src * Alpha + Dst * (1 - Alpha)
					r = (uint8_t)(r * a + br * (1 - a));
					g = (uint8_t)(g * a + bg_ * (1 - a));
					b = (uint8_t)(b * a + bb * (1 - a));
					g_frameBuffer[idx] = (255 << 24) | (r << 16) | (g << 8) | b;
				}
				else {
					g_frameBuffer[idx] = (255 << 24) | (r << 16) | (g << 8) | b;
					g_zBuffer[idx] = z; // 只有不透明物体才更新 Z-Buffer
				}
			}
		}
	}
}

// ==========================================
// 7. 光线跟踪渲染器
// ==========================================

// 定义光线结构：在 3D 空间中由一个起点和一个无限延伸的方向向量组成
struct Ray {
	Vec3 origin; // 光线的起点（通常是摄像机位置或反射撞击点）
	Vec3 dir;    // 光线的方向向量（必须归一化以确保距离计算准确）
};

// 定义交点详细信息：用于在光线射中物体后，传递给着色器进行光照计算
struct HitInfo {
	float t;          // 距离参数：光线方程 P = O + tD 中的 t。表示从起点到交点的距离
	Vec3 pos;         // 撞击点的世界空间坐标
	Vec3 normal;      // 撞击点的法向量（用于光照计算，由顶点法线插值得到）
	Vec2 uv;          // 撞击点的纹理坐标（用于纹理采样）
	int meshIdx;      // 物体索引：标记射中了场景中哪个 Mesh
	bool isTransp;    // 类型标记：true 代表撞击的是透明网格，false 代表普通网格
};

/**
 * Möller-Trumbore 射线-三角形相交算法
 * ---------------------------------------------------------
 * 该算法是图形学中最著名的求交算法之一，它不计算平面方程，直接求出重心坐标。
 * @param t [out] 输出光线运行的距离
 * @param u, v [out] 输出交点在三角形上的重心坐标，用于插值法线和 UV
 */
bool rayTriangleIntersect(const Ray& ray, const Vec3& v0, const Vec3& v1, const Vec3& v2, float& t, float& u, float& v) {
	// 1. 计算三角形的两条边向量
	Vec3 e1 = v1 - v0;
	Vec3 e2 = v2 - v0;

	// 2. 开始克莱姆法则求解线性方程组
	// h 向量是光线方向与边 e2 的叉积
	Vec3 h = ray.dir.cross(e2);

	// a 为 determinant（行列式），如果 a 接近 0，说明光线与三角形平面平行
	float a = e1.dot(h);
	if (std::abs(a) < 1e-6f) return false;

	float f = 1.0f / a;
	Vec3 s = ray.origin - v0; // 计算起点到顶点 v0 的距离向量

	// 3. 计算重心坐标 u
	// u 代表交点在 e1 方向上的分量
	u = f * s.dot(h);
	if (u < 0.0f || u > 1.0f) return false; // u 必须在 [0, 1] 之间

	// 4. 计算重心坐标 v
	// q 是 s 向量与边 e1 的叉积
	Vec3 q = s.cross(e1);
	v = f * ray.dir.dot(q);
	// v 必须非负，且 u + v 不能超过 1（三角形定义的约束）
	if (v < 0.0f || u + v > 1.0f) return false;

	// 5. 计算距离参数 t
	// t 代表光线沿 dir 方向前进多少距离后发生碰撞
	t = f * e2.dot(q);

	// 返回 t > 0 确保碰撞发生在光线前方，0.001f 用于防止自相交（自影）现象
	return t > 0.001f;
}

/**
 * AABB (轴对齐包围盒) 相交检测 - Slabs 算法
 * ---------------------------------------------------------
 * 原理：将盒子看作三对平行的平面（ slabs ），计算光线穿过这三对平面所用的时间区间。
 * 只有当三组时间的交集不为空时，光线才真正穿过盒子。
 */
bool intersectAABB(const Ray& ray, const Vec3& minB, const Vec3& maxB) {
	// 处理 X 轴：计算光线进入和离开盒子 X 范围的时间
	float tmin = (minB.x - ray.origin.x) / ray.dir.x;
	float tmax = (maxB.x - ray.origin.x) / ray.dir.x;
	if (tmin > tmax) std::swap(tmin, tmax); // 确保 tmin 是进入时间

	// 处理 Y 轴
	float tymin = (minB.y - ray.origin.y) / ray.dir.y;
	float tymax = (maxB.y - ray.origin.y) / ray.dir.y;
	if (tymin > tymax) std::swap(tymin, tymax);

	// 如果 X 轴的进入时间晚于 Y 轴的离开时间，或者 Y 的进入晚于 X 的离开，则不相交
	if ((tmin > tymax) || (tymin > tmax)) return false;

	// 更新总体的进入和离开时间
	if (tymin > tmin) tmin = tymin;
	if (tymax < tmax) tmax = tymax;

	// 处理 Z 轴
	float tzmin = (minB.z - ray.origin.z) / ray.dir.z;
	float tzmax = (maxB.z - ray.origin.z) / ray.dir.z;
	if (tzmin > tzmax) std::swap(tzmin, tzmax);

	// 最后的逻辑冲突判断
	if ((tmin > tzmax) || (tzmin > tmax)) return false;

	return true; // 存在重叠的时间段，说明光线穿过了包围盒
}

/**
 * 核心追踪函数：遍历场景中的所有物体，找出第一个被射中的点
 */
bool traceRay(const Ray& ray, HitInfo& hit) {
	hit.t = 1e30f; // 初始化为一个极大的距离，方便查找最小的 t（最近交点）
	bool found = false;

	// 1. 遍历不透明物体列表
	for (int mi = 0; mi < (int)g_objects.size(); mi++) {
		const Mesh& m = g_objects[mi];

		// 【加速结构】：如果射线连物体的 AABB 包围盒都没碰到，直接跳过其内部数千个三角形
		if (!intersectAABB(ray, m.minWorldBounds, m.maxWorldBounds)) continue;

		// 遍历该模型的所有三角形索引
		for (size_t i = 0; i < m.indices.size(); i += 3) {
			// 将顶点的局部坐标通过变换矩阵转到世界空间
			Vec4 wp0 = m.transform * Vec4(m.verts[m.indices[i]].pos, 1);
			Vec4 wp1 = m.transform * Vec4(m.verts[m.indices[i + 1]].pos, 1);
			Vec4 wp2 = m.transform * Vec4(m.verts[m.indices[i + 2]].pos, 1);
			Vec3 v0(wp0.x, wp0.y, wp0.z), v1(wp1.x, wp1.y, wp1.z), v2(wp2.x, wp2.y, wp2.z);

			float t, u, v;
			// 如果撞击成功且距离比目前记录的更近
			if (rayTriangleIntersect(ray, v0, v1, v2, t, u, v) && t < hit.t) {
				hit.t = t;
				hit.meshIdx = mi;
				hit.isTransp = false;
				hit.pos = ray.origin + ray.dir * t; // 计算精确的交点坐标

				// 重心插值：利用 u,v 计算交点处的平滑法线（Phong Shading 效果）
				hit.normal = ((1 - u - v) * m.verts[m.indices[i]].normal +
					u * m.verts[m.indices[i + 1]].normal +
					v * m.verts[m.indices[i + 2]].normal).normalize();

				// 重心插值：计算交点的 UV 坐标，用于后续着色时的纹理采样
				hit.uv.x = (1 - u - v) * m.verts[m.indices[i]].uv.x + u * m.verts[m.indices[i + 1]].uv.x + v * m.verts[m.indices[i + 2]].uv.x;
				hit.uv.y = (1 - u - v) * m.verts[m.indices[i]].uv.y + u * m.verts[m.indices[i + 1]].uv.y + v * m.verts[m.indices[i + 2]].uv.y;
				found = true;
			}
		}
	}

	// 2. 遍历透明物体列表（逻辑完全相同，但会标记 isTransp = true）
	for (int mi = 0; mi < (int)g_transp_objects.size(); mi++) {
		const Mesh& m = g_transp_objects[mi];
		if (!intersectAABB(ray, m.minWorldBounds, m.maxWorldBounds)) continue;

		for (size_t i = 0; i < m.indices.size(); i += 3) {
			Vec4 wp0 = m.transform * Vec4(m.verts[m.indices[i]].pos, 1);
			Vec4 wp1 = m.transform * Vec4(m.verts[m.indices[i + 1]].pos, 1);
			Vec4 wp2 = m.transform * Vec4(m.verts[m.indices[i + 2]].pos, 1);
			Vec3 v0(wp0.x, wp0.y, wp0.z), v1(wp1.x, wp1.y, wp1.z), v2(wp2.x, wp2.y, wp2.z);

			float t, u, v;
			if (rayTriangleIntersect(ray, v0, v1, v2, t, u, v) && t < hit.t) {
				hit.t = t; hit.meshIdx = mi; hit.isTransp = true;
				hit.pos = ray.origin + ray.dir * t;
				hit.normal = ((1 - u - v) * m.verts[m.indices[i]].normal + u * m.verts[m.indices[i + 1]].normal + v * m.verts[m.indices[i + 2]].normal).normalize();
				hit.uv.x = (1 - u - v) * m.verts[m.indices[i]].uv.x + u * m.verts[m.indices[i + 1]].uv.x + v * m.verts[m.indices[i + 2]].uv.x;
				hit.uv.y = (1 - u - v) * m.verts[m.indices[i]].uv.y + u * m.verts[m.indices[i + 1]].uv.y + v * m.verts[m.indices[i + 2]].uv.y;
				found = true;
			}
		}
	}
	return found;
}

/**
 * 光线跟踪着色 (Recursive Shading)
 * ---------------------------------------------------------
 * @param depth 当前递归深度。每发生一次反射，深度+1。防止光线在镜面间无限跳跃。
 */
Vec3 shadeRT(const HitInfo& hit, const Vec3& camPos, const Vec3& rayDir, int depth) {
	if (depth > 3) return Vec3(0.1f); // 递归出口：超过3次反射则只返回极弱的基础光

	// 获取物体材质
	const Mesh& mesh = hit.isTransp ? g_transp_objects[hit.meshIdx] : g_objects[hit.meshIdx];
	// 基础颜色 = 纹理采样值 * 材质颜色
	Vec3 texColor = (mesh.texID >= 0) ? sampleTexture(mesh.texID, hit.uv) * mesh.color : mesh.color;

	Vec3 N = hit.normal.normalize();
	Vec3 V = (camPos - hit.pos).normalize(); // 视线方向（指向相机）
	Vec3 result(0, 0, 0);

	// 遍历场景光源进行直接光照计算
	for (const auto& light : g_lights) {
		Vec3 L; float atten = 1.0f;
		// 计算光源方向 L 和 距离衰减 atten
		if (light.type == LIGHT_DIRECTIONAL) {
			L = (Vec3(0, 0, 0) - light.dir).normalize();
		}
		else {
			L = (light.pos - hit.pos).normalize();
			float d = (light.pos - hit.pos).length();
			atten = 1.0f / (1 + 0.09f * d + 0.032f * d * d); // 二次衰减公式
		}

		// 【Shadow Ray - 阴影检测】：从交点向光源发射一条射线
		// 如果射中任何物体，说明该点处于阴影中
		Ray shadowRay;
		shadowRay.origin = hit.pos + N * 0.01f; // 沿法线偏移一点点，防止“自相交”错误
		shadowRay.dir = L;
		HitInfo shadowHit;
		float shadow = traceRay(shadowRay, shadowHit) ? 0.3f : 1.0f; // 阴影区保留30%亮度

		// 漫反射 (Diffuse) + 镜面高光 (Specular)
		float diff = std::max(N.dot(L), 0.0f);
		Vec3 H = (L + V).normalize();
		float spec = std::pow(std::max(N.dot(H), 0.0f), mesh.shininess);

		result = result + light.color * (diff + spec * 0.5f) * light.intensity * atten * shadow;
	}

	// 环境光模拟：基础 0.3 + 向上方向的微弱天光
	float amb = 0.3f + std::max(N.dot(Vec3(0, 1, 0)), 0.0f) * 0.1f;
	result = result * texColor + texColor * amb;

	// 【递归反射计算】：如果物体有透明/镜面属性
	if (mesh.isTransparent && depth < 2) {
		// 计算反射方向：R = I - 2(I·N)N
		Vec3 R = rayDir - N * 2.0f * rayDir.dot(N);
		Ray reflRay;
		reflRay.origin = hit.pos + N * 0.01f;
		reflRay.dir = R.normalize();
		HitInfo reflHit;

		// 递归调用：追踪反射光线射中的颜色
		if (traceRay(reflRay, reflHit)) {
			Vec3 reflColor = shadeRT(reflHit, camPos, reflRay.dir, depth + 1);
			// 混合结果：当前颜色占 70%，反射颜色占 30%
			result = result * 0.7f + reflColor * 0.3f;
		}
	}
	return result;
}

/**
 * 渲染主入口：生成离线图像
 */
void renderRaytraced(Vec3 camPos, Vec3 camDir, const std::string& filename) {
	std::cout << "Rendering raytraced image: " << filename << "..." << std::endl;
	// 分配图像内存 (800x600 * 3 字节)
	std::vector<uint8_t> image(WIDTH * HEIGHT * 3);

	// 1. 建立相机本地坐标轴 (LookAt 逻辑)
	Vec3 up(0, 1, 0);
	Vec3 right = camDir.cross(up).normalize(); // 右向量
	Vec3 camUp = right.cross(camDir).normalize(); // 正确的上向量

	float fov = PI / 3.5f; // 视野范围
	float aspect = (float)WIDTH / HEIGHT; // 宽高比

	// 2. 遍历图像平面的每一个像素
#pragma omp parallel for
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			// 将像素位置 (x, y) 映射到规范化的相机平面 [-1, 1]
			// px, py 代表了该像素在相机视野中的偏移角度
			float px = (2.0f * (x + 0.5f) / WIDTH - 1.0f) * aspect * tan(fov / 2);
			float py = (1.0f - 2.0f * (y + 0.5f) / HEIGHT) * tan(fov / 2);

			// 构造射线：从相机位置出发，穿过当前像素点
			Ray ray;
			ray.origin = camPos;
			ray.dir = (camDir + right * px + camUp * py).normalize();

			HitInfo hit;
			Vec3 color(0.12f, 0.12f, 0.14f); // 默认背景颜色（星空灰）

			// 3. 执行场景求交
			if (traceRay(ray, hit)) {
				// 4. 执行递归着色
				color = shadeRT(hit, camPos, ray.dir, 0);
			}

			// 5. 将计算出的浮点颜色 [0, 1] 转换为 8-bit [0, 255] 存储
			int idx = (y * WIDTH + x) * 3;
			image[idx] = (uint8_t)std::min(255.0f, std::max(0.0f, color.x) * 255);
			image[idx + 1] = (uint8_t)std::min(255.0f, std::max(0.0f, color.y) * 255);
			image[idx + 2] = (uint8_t)std::min(255.0f, std::max(0.0f, color.z) * 255);
		}

		// 渲染进度条：每 50 行打印一次
		if (y % 50 == 0) {
			std::cout << " Progress: " << (static_cast<float>(y) * 100.0f / HEIGHT) << "%" << std::endl;
		}
	}

	// 6. 使用 stb_image_write 将内存数据存为 PNG 格式
	if (stbi_write_png(filename.c_str(), WIDTH, HEIGHT, 3, image.data(), WIDTH * 3)) {
		std::cout << "[OK] Saved: " << filename << std::endl;
	}
	else {
		std::cerr << "[ERROR] Failed to save PNG: " << filename << std::endl;
	}
}

// ==========================================
// 场景初始化
// ==========================================
void addMeshToGroup(SceneObject& obj, Mesh& m, Vec3 localPos) {
	m.localOffset = localPos;
	int idx = (int)g_objects.size();
	g_objects.push_back(m);
	obj.meshIndices.push_back(idx);
}

void initScene() {
	generateTextures();

	// 创建一个巨大的房间，参数最后加上 true，表示翻转法线
	// 假设房间大小为 40x40x40，包住整个场景
	Mesh room = createBox(Vec3(0, 20, 0), Vec3(40, 40, 40), 0, Vec3(0.85f, 0.85f, 0.82f), 63, false, 1.0f, 16.0f, true);
	room.castShadow = false; //天花板不再挡光
	g_objects.push_back(room);

	// === 桌子 ===
	{
		SceneObject table; table.name = "Table"; table.position = Vec3(0, 0, 0); table.rotation = 0;
		Vec3 woodColor(0.85f, 0.75f, 0.55f); 
		Mesh top = createBox(Vec3(0, 3.8f, 0), Vec3(6, 0.2f, 4.5f), -1, woodColor, 63, false, 1.0f, 32.0f);
		addMeshToGroup(table, top, Vec3(0, 3.8f, 0));
		Vec3 ls(0.3f, 3.8f, 0.3f);
		Mesh leg1 = createBox(Vec3(-2.8f, 1.9f, -2.0f), ls, -1, woodColor); addMeshToGroup(table, leg1, Vec3(-2.8f, 1.9f, -2.0f));
		Mesh leg2 = createBox(Vec3(2.8f, 1.9f, -2.0f), ls, -1, woodColor); addMeshToGroup(table, leg2, Vec3(2.8f, 1.9f, -2.0f));
		Mesh leg3 = createBox(Vec3(-2.8f, 1.9f, 2.0f), ls, -1, woodColor); addMeshToGroup(table, leg3, Vec3(-2.8f, 1.9f, 2.0f));
		Mesh leg4 = createBox(Vec3(2.8f, 1.9f, 2.0f), ls, -1, woodColor); addMeshToGroup(table, leg4, Vec3(2.8f, 1.9f, 2.0f));
		g_sceneObjects.push_back(table);
	}

	// === 椅子1-4 ===
	auto createChair = [&](Vec3 pos, float angle, const std::string& name) {
		SceneObject chair; chair.name = name; chair.position = pos; chair.rotation = angle;
		Vec3 woodColor(0.85f, 0.75f, 0.55f); 
		auto addPart = [&](Vec3 p, Vec3 s) {
			Mesh m = createBox(p, s, -1, woodColor, 63, false, 1.0f, 16.0f);
			addMeshToGroup(chair, m, p);
			};
		addPart(Vec3(0, 2.0f, 0), Vec3(2.1f, 0.2f, 2.1f));
		addPart(Vec3(0, 3.8f, -0.95f), Vec3(2.1f, 1.2f, 0.15f));
		addPart(Vec3(-0.85f, 3.0f, -0.95f), Vec3(0.25f, 2.0f, 0.15f));
		addPart(Vec3(0.85f, 3.0f, -0.95f), Vec3(0.25f, 2.0f, 0.15f));
		addPart(Vec3(-0.85f, 1.0f, 0.85f), Vec3(0.3f, 2.0f, 0.3f));
		addPart(Vec3(0.85f, 1.0f, 0.85f), Vec3(0.3f, 2.0f, 0.3f));
		addPart(Vec3(-0.85f, 1.0f, -0.85f), Vec3(0.3f, 2.0f, 0.3f));
		addPart(Vec3(0.85f, 1.0f, -0.85f), Vec3(0.3f, 2.0f, 0.3f));
		chair.updateTransforms(g_objects, g_transp_objects);
		g_sceneObjects.push_back(chair);
		};
	createChair(Vec3(0, 0, -3.5f), 0, "Chair1");
	createChair(Vec3(-4.5f, 0, 0.5f), 1.57f, "Chair2");
	createChair(Vec3(4.5f, 0, 0.5f), -1.57f, "Chair3");
	createChair(Vec3(0, 0, 3.5f), 3.14159f, "Chair4");

	// === 电视 ===
	{
		SceneObject tv; tv.name = "TV"; tv.position = Vec3(-1.8f, 4.8f, -0.5f); tv.rotation = 0;
		Mesh body = createBox(Vec3(0, 0, -0.5f), Vec3(2.0f, 1.6f, 1.8f), -1, Vec3(0.4f, 0.4f, 0.45f), 63, false, 1.0f, 64.0f);
		addMeshToGroup(tv, body, Vec3(0, 0, -0.5f));
		Mesh screen = createBox(Vec3(0, 0, 0.42f), Vec3(1.2f, 1.0f, 0.1f), 2, Vec3(1, 1, 1), 63, false, 1.0f, 128.0f);
		addMeshToGroup(tv, screen, Vec3(0, 0, 0.42f));
		tv.updateTransforms(g_objects, g_transp_objects);
		g_sceneObjects.push_back(tv);
	}

	// === 鱼缸 ===
	{
		SceneObject tank; tank.name = "FishTank"; tank.position = Vec3(1.5f, 4.4f, 0.0f); tank.rotation = 0;
		Mesh glass = createBox(Vec3(0, 0, 0), Vec3(1.2f, 0.9f, 0.9f), -1, Vec3(0.8f, 0.9f, 0.95f), 59, true, 0.15f, 128.0f);
		glass.localOffset = Vec3(0, 0, 0);
		int idx1 = (int)g_transp_objects.size();
		g_transp_objects.push_back(glass);

		Mesh water = createBox(Vec3(0, -0.15f, 0), Vec3(1.1f, 0.6f, 0.8f), -1, Vec3(0.2f, 0.4f, 0.9f), 63, true, 0.3f, 64.0f);
		water.localOffset = Vec3(0, -0.15f, 0);
		g_transp_objects.push_back(water);

		tank.meshIndices.push_back(-1 - idx1);
		tank.meshIndices.push_back(-1 - (idx1 + 1));
		tank.updateTransforms(g_objects, g_transp_objects);
		g_sceneObjects.push_back(tank);
	}

	// === 吊灯 ===
	{
		SceneObject lamp; lamp.name = "Lamp"; lamp.position = Vec3(0, 7.0f, 0); lamp.rotation = 0;
		Mesh rod = createBox(Vec3(0, 1.0f, 0), Vec3(0.05f, 4.0f, 0.05f), -1, Vec3(0.1f, 0.1f, 0.1f));
		rod.castShadow = false; // 【新增】吊灯杆子不投影
		addMeshToGroup(lamp, rod, Vec3(0, 1.0f, 0));

		Mesh shade = createLampshade(Vec3(0, -1.0f, 0), 0.4f, 1.2f, 0.8f, Vec3(0.9f, 0.9f, 0.85f));
		rod.castShadow = false; // 【新增】吊灯罩子不投影
		addMeshToGroup(lamp, shade, Vec3(0, -1.0f, 0));

		// 在吊灯罩子里塞一个暖黄色的发光小方块（模拟灯泡）
		Mesh bulb = createBox(Vec3(0, -0.2f, 0), Vec3(0.3f, 0.3f, 0.3f), -1, Vec3(1.0f, 0.9f, 0.5f), 63, false, 1.0f, 128.0f);
		bulb.castShadow = false; // 灯泡自己绝对不能挡光
		addMeshToGroup(lamp, bulb, Vec3(0, -0.2f, 0));

		lamp.updateTransforms(g_objects, g_transp_objects);
		g_sceneObjects.push_back(lamp);
	}

	// === 罗小黑模型 ===
	{
		SceneObject lxh; lxh.name = "LuoXiaoHei"; lxh.position = Vec3(4.0f, 0.0f, 4.5f); lxh.rotation = 0.87f;
		std::cout << "\n========== Loading Model ==========" << std::endl;
		Mesh objModel = loadOBJ("lxh.obj", Vec3(0, 0, 0), Vec3(2.0f, 2.0f, 2.0f));
		objModel.localOffset = Vec3(0, 0, 0);
		if (objModel.texID < 0) objModel.texID = 1;
		addMeshToGroup(lxh, objModel, Vec3(0, 0, 0));
		lxh.updateTransforms(g_objects, g_transp_objects);
		g_sceneObjects.push_back(lxh);
		std::cout << "========== Model Loaded ==========" << std::endl << std::endl;
	}

	// 3 光源
	g_lights.clear();

	// 1. 阳光：改为暖金色，方向更倾斜，拉长影子的纵深感
	Light dirLight; dirLight.type = LIGHT_DIRECTIONAL;
	dirLight.dir = Vec3(-0.4f, -0.8f, -0.4f).normalize();
	dirLight.color = Vec3(1.0f, 0.95f, 0.85f); // 暖金色的阳光
	dirLight.intensity = 1.4f; // 增强阳光亮度
	g_lights.push_back(dirLight);

	// 2. 桌面补光：改为温暖的橘黄色，假装是灯泡发出的光
	Light pointLight; pointLight.type = LIGHT_POINT;
	pointLight.pos = Vec3(0, 4.0f, 0); // 稍微往下放一点，贴近桌面
	pointLight.color = Vec3(1.0f, 0.6f, 0.2f); // 橘黄色
	pointLight.intensity = 0.7f;
	g_lights.push_back(pointLight);

	// 3. 电视屏幕光：改为冰蓝色，增加冷暖对比
	Light spotLight; spotLight.type = LIGHT_SPOT;
	spotLight.pos = Vec3(-1.8f, 4.8f, -0.4f);
	spotLight.dir = Vec3(1.0f, -0.2f, 0.0f).normalize();
	spotLight.color = Vec3(0.3f, 0.6f, 1.0f); // 冰蓝色
	spotLight.intensity = 1.2f;
	g_lights.push_back(spotLight);

	std::cout << "Scene objects: " << g_sceneObjects.size() << " (Table, 4 Chairs, TV, FishTank, Lamp, LuoXiaoHei)" << std::endl;
}

int main(int argc, char* argv[]) {
	// --- 1. SDL 基础环境初始化 ---
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("3D Scene | 1:Camera 2:Object 3:Light | Tab:Select | R:Rotate | F1-F3:Raytracing", WIDTH, HEIGHT, 0);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
	// 创建流式纹理，用于将 CPU 计算好的 frameBuffer 直接推送到 GPU 显示
	SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	initScene(); // 初始化场景中的网格数据、纹理加载、场景结构

	// --- 2. 状态变量定义 ---
	Vec3 camPos(10, 11, 12);           // 摄像机初始位置
	float yaw = -2.2f, pitch = -0.5f;  // 相机偏航角和俯仰角
	int selectedObjIdx = -1;           // 当前选中的场景物体索引
	int selectedLightIdx = 0;          // 当前选中的光源索引
	InteractionMode mode = MODE_CAMERA; // 初始操作模式：摄像机
	bool running = true;               // 程序运行状态
	Uint64 lastTime = SDL_GetTicks();  // 用于计算 deltaTime
	bool mouseDown = false;            // 记录鼠标按键状态，用于处理拖拽逻辑

	// 控制台操作提示
	std::cout << "\n=== Controls ===" << std::endl;
	std::cout << "1: Camera mode | 2: Object mode | 3: Light mode" << std::endl;
	std::cout << "Tab: Select next object/light" << std::endl;
	std::cout << "WASD/QE: Move camera | Arrow keys: Rotate view" << std::endl;
	std::cout << "R + Mouse: Rotate selected object" << std::endl;
	std::cout << "Mouse drag: Move selected object/light" << std::endl;
	std::cout << "+/-: Adjust light intensity" << std::endl;
	std::cout << "F1/F2/F3: Render raytraced images from preset views" << std::endl;
	std::cout << "================\n" << std::endl;

	// --- 3. 渲染主循环 ---
	while (running) {
		// 计算两帧之间的时间差（Delta Time），确保在不同性能的机器上移动速度一致
		Uint64 currentTime = SDL_GetTicks();
		float deltaTime = (currentTime - lastTime) / 1000.0f;
		lastTime = currentTime;
		if (deltaTime > 0.1f) deltaTime = 0.1f; // 限制最大步长，防止跳帧导致的逻辑飞出

		// --- 4. 事件处理逻辑 (键盘与鼠标点击) ---
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) running = false;
			if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) mouseDown = true;
			if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) mouseDown = false;

			if (e.type == SDL_EVENT_KEY_DOWN) {
				if (e.key.key == SDLK_ESCAPE) running = false;
				// 模式切换：1, 2, 3 分别对应相机、物体、光源模式
				if (e.key.key == SDLK_1) { mode = MODE_CAMERA; std::cout << "Mode: Camera" << std::endl; }
				if (e.key.key == SDLK_2) { mode = MODE_OBJECT; std::cout << "Mode: Object" << std::endl; }
				if (e.key.key == SDLK_3) { mode = MODE_LIGHT; std::cout << "Mode: Light" << std::endl; }

				// TAB 键：在物体或光源之间切换选中状态
				if (e.key.key == SDLK_TAB) {
					if (mode == MODE_OBJECT) {
						selectedObjIdx++;
						if (selectedObjIdx >= (int)g_sceneObjects.size()) selectedObjIdx = -1; // 循环或取消选择
						if (selectedObjIdx >= 0) std::cout << "Selected: " << g_sceneObjects[selectedObjIdx].name << std::endl;
						else std::cout << "Selection cleared" << std::endl;
					}
					else if (mode == MODE_LIGHT) {
						selectedLightIdx = (selectedLightIdx + 1) % g_lights.size();
						std::cout << "Selected light: " << selectedLightIdx << std::endl;
					}
				}

				// 光源参数调整：+/- 键调整强度
				if (mode == MODE_LIGHT) {
					if (e.key.key == SDLK_EQUALS || e.key.key == SDLK_KP_PLUS) {
						g_lights[selectedLightIdx].intensity += 0.2f;
					}
					if (e.key.key == SDLK_MINUS || e.key.key == SDLK_KP_MINUS) {
						g_lights[selectedLightIdx].intensity = std::max(0.1f, g_lights[selectedLightIdx].intensity - 0.2f);
					}
				}

				// F1-F3 键：触发离线光线跟踪渲染并保存为图片
				if (e.key.key == SDLK_F1) renderRaytraced(Vec3(8, 10, 10), (Vec3(0, 3, 0) - Vec3(8, 10, 10)).normalize(), "raytraced_view1.png");
				if (e.key.key == SDLK_F2) renderRaytraced(Vec3(-10, 6, 5), (Vec3(0, 3, 0) - Vec3(-10, 6, 5)).normalize(), "raytraced_view2.png");
				if (e.key.key == SDLK_F3) {
					Vec3 currentFwd(cos(pitch) * cos(yaw), sin(pitch), cos(pitch) * sin(yaw));
					renderRaytraced(camPos, currentFwd, "raytraced_view3.png");
				}
			}

			// --- 5. 鼠标拖拽逻辑 (关键交互) ---
			if (e.type == SDL_EVENT_MOUSE_MOTION && mouseDown) {
				float dx = e.motion.xrel * 0.05f; // 鼠标水平移动偏移
				float dy = e.motion.yrel * 0.05f; // 鼠标垂直移动偏移

				// 物体模式交互
				if (mode == MODE_OBJECT && selectedObjIdx >= 0) {
					const bool* keys = SDL_GetKeyboardState(NULL);
					// 按住 R 拖拽鼠标：旋转物体
					if (keys[SDL_SCANCODE_R]) {
						g_sceneObjects[selectedObjIdx].rotation += dx * 0.5f;
						g_sceneObjects[selectedObjIdx].updateTransforms(g_objects, g_transp_objects);
					}
					// 直接拖拽鼠标：相对于当前视角平面平移物体
					else {
						// 算法：根据相机的偏航角 yaw 计算出在世界空间中的位移向量
						// 使得物体的移动方向始终符合用户的屏幕视觉直觉（左右拖动则左右移动）
						Vec3 moveVec = Vec3(-dx * sin(yaw) - dy * cos(yaw), 0, dx * cos(yaw) - dy * sin(yaw));
						g_sceneObjects[selectedObjIdx].position = g_sceneObjects[selectedObjIdx].position + moveVec;
						g_sceneObjects[selectedObjIdx].updateTransforms(g_objects, g_transp_objects);
					}
				}
				// 光源模式交互
				else if (mode == MODE_LIGHT) {
					Vec3 moveVec = Vec3(-dx * sin(yaw) - dy * cos(yaw), 0, dx * cos(yaw) - dy * sin(yaw));
					if (g_lights[selectedLightIdx].type != LIGHT_DIRECTIONAL) {
						// 点光源/聚光灯：直接移动其在空间的位置
						g_lights[selectedLightIdx].pos = g_lights[selectedLightIdx].pos + moveVec;
					}
					else {
						// 平行光：由于位置不固定，这里通过鼠标位移修改其照射方向 dir
						g_lights[selectedLightIdx].dir.x += dx * 0.1f;
						g_lights[selectedLightIdx].dir.z += dy * 0.1f;
						g_lights[selectedLightIdx].dir = g_lights[selectedLightIdx].dir.normalize();
					}
				}
			}
		}

		// --- 6. 实时键盘轮询 (相机移动逻辑) ---
		const bool* keys = SDL_GetKeyboardState(NULL);
		// 计算当前相机的前向 (Forward) 和 右侧 (Right) 向量
		Vec3 fwd(cos(pitch) * cos(yaw), sin(pitch), cos(pitch) * sin(yaw));
		Vec3 rt = fwd.cross(Vec3(0, 1, 0)).normalize();

		float moveSpeed = 8.0f * deltaTime;
		float rotSpeed = 2.0f * deltaTime;

		// WASD/QE 相机位置控制
		if (keys[SDL_SCANCODE_W]) camPos = camPos + fwd * moveSpeed;
		if (keys[SDL_SCANCODE_S]) camPos = camPos - fwd * moveSpeed;
		if (keys[SDL_SCANCODE_A]) camPos = camPos - rt * moveSpeed;
		if (keys[SDL_SCANCODE_D]) camPos = camPos + rt * moveSpeed;
		if (keys[SDL_SCANCODE_Q]) camPos.y -= moveSpeed;
		if (keys[SDL_SCANCODE_E]) camPos.y += moveSpeed;
		// 方向键视角控制
		if (keys[SDL_SCANCODE_LEFT]) yaw -= rotSpeed;
		if (keys[SDL_SCANCODE_RIGHT]) yaw += rotSpeed;
		if (keys[SDL_SCANCODE_UP]) pitch += rotSpeed;
		if (keys[SDL_SCANCODE_DOWN]) pitch -= rotSpeed;
		// 限制俯仰角，防止相机“翻车”
		pitch = std::max(-1.5f, std::min(1.5f, pitch));

		// --- 7. 渲染流程 第一阶段：阴影图生成 (Shadow Pass) ---
		// 逻辑：以光源 0 为视点，渲染所有不透明和透明物体到 g_shadowMap 缓冲区
		std::fill(g_shadowMap.begin(), g_shadowMap.end(), 1.0f);
		// 计算光源视点：将定向光看作是 20 米外的一个摄像头
		Vec3 lightPos = Vec3(0, 0, 0) - g_lights[0].dir * 40.0f; // 假定一个足够远的光源点
		// 计算光源投影矩阵 (LP)：正交投影 (15x15x50 的盒子) * 视图变换 (LookAt)
		Mat4 lp = Mat4::ortho(-20, 20, -20, 20, 1, 80) * Mat4::lookAt(lightPos, Vec3(0, 0, 0), Vec3(0, 1, 0));

		// 渲染不透明物体到阴影贴图
		for (const auto& obj : g_objects) {
			if (!obj.castShadow) continue; //如果物体不投影，直接跳过
			for (size_t i = 0; i < obj.indices.size(); i += 3) {
				VSOut v0, v1, v2;
				v0.pos = shadowVS(obj.verts[obj.indices[i]], obj.transform, lp);
				v1.pos = shadowVS(obj.verts[obj.indices[i + 1]], obj.transform, lp);
				v2.pos = shadowVS(obj.verts[obj.indices[i + 2]], obj.transform, lp);
				drawShadow(v0, v1, v2);
			}
		}

		// 渲染透明物体到阴影贴图（注：简单实现中透明物体也会产生实心阴影）
		for (const auto& obj : g_transp_objects) {
			for (size_t i = 0; i < obj.indices.size(); i += 3) {
				VSOut v0, v1, v2;
				v0.pos = shadowVS(obj.verts[obj.indices[i]], obj.transform, lp);
				v1.pos = shadowVS(obj.verts[obj.indices[i + 1]], obj.transform, lp);
				v2.pos = shadowVS(obj.verts[obj.indices[i + 2]], obj.transform, lp);
				drawShadow(v0, v1, v2);
			}
		}

		//8. 渲染流程 第二阶段：主场景渲染 (Color Pass)
		std::fill(g_zBuffer.begin(), g_zBuffer.end(), 1.0f); // 重置深度图
		std::fill(g_frameBuffer.begin(), g_frameBuffer.end(), 0xFF202025); // 背景色（深灰蓝色）

		// 构建透视矩阵和视图矩阵 (VP)
		Mat4 vp = Mat4::perspective(PI / 3.5f, (float)WIDTH / HEIGHT, 0.1f, 100.0f) * Mat4::lookAt(camPos, camPos + fwd, Vec3(0, 1, 0));

		// (1) 渲染不透明物体
		for (int i = 0; i < (int)g_objects.size(); ++i) {
			bool isSelected = (selectedObjIdx >= 0) ? g_sceneObjects[selectedObjIdx].containsMesh(i) : false;
			for (size_t j = 0; j < g_objects[i].indices.size(); j += 3) {

				// 第一步：执行顶点着色器，把3个顶点算出来存好
				VSOut v0 = mainVS(g_objects[i].verts[g_objects[i].indices[j]], g_objects[i].transform, vp);
				VSOut v1 = mainVS(g_objects[i].verts[g_objects[i].indices[j + 1]], g_objects[i].transform, vp);
				VSOut v2 = mainVS(g_objects[i].verts[g_objects[i].indices[j + 2]], g_objects[i].transform, vp);

				// 第二步：执行裁剪！把这3个顶点扔进裁剪函数
				// 如果三角形越界了，它可能会被切成多个新的小三角形退回来
				std::vector<VSOut> clippedTris = clipTriangleAgainstNearPlane(v0, v1, v2);

				// 第三步：拿切好的新三角形，送去光栅化填像素
				// 每次取3个顶点画一个三角形
				for (size_t k = 0; k < clippedTris.size(); k += 3) {
					drawTriangleMain(
						clippedTris[k], clippedTris[k + 1], clippedTris[k + 2],
						g_objects[i], g_lights, camPos, lp, isSelected
					);
				}

			}
		}

		// (2) 渲染透明物体：画家算法 (Painter's Algorithm)
		// 逻辑：透明物体必须按从远到近的顺序渲染，才能实现正确的混合叠加效果
		std::vector<int> transpIndices(g_transp_objects.size());
		for (int i = 0; i < (int)g_transp_objects.size(); ++i) transpIndices[i] = i;
		std::sort(transpIndices.begin(), transpIndices.end(), [&](int a, int b) {
			float da = (g_transp_objects[a].posOffset - camPos).length();
			float db = (g_transp_objects[b].posOffset - camPos).length();
			return da > db; // 远者优先
			});

		for (int i : transpIndices) {
			bool isSelected = (selectedObjIdx >= 0) ? g_sceneObjects[selectedObjIdx].containsMesh(-1 - i) : false;
			const auto& obj = g_transp_objects[i];

			for (size_t j = 0; j < obj.indices.size(); j += 3) {

				// 第一步：执行顶点着色器，获取齐次裁剪空间下的三个顶点
				VSOut v0 = mainVS(obj.verts[obj.indices[j]], obj.transform, vp);
				VSOut v1 = mainVS(obj.verts[obj.indices[j + 1]], obj.transform, vp);
				VSOut v2 = mainVS(obj.verts[obj.indices[j + 2]], obj.transform, vp);

				// 第二步：执行近裁剪面裁剪
				std::vector<VSOut> clippedTris = clipTriangleAgainstNearPlane(v0, v1, v2);

				// 第三步：遍历裁剪后生成的新三角形，进行光栅化和Alpha混合绘制
				for (size_t k = 0; k < clippedTris.size(); k += 3) {
					drawTriangleMain(
						clippedTris[k], clippedTris[k + 1], clippedTris[k + 2],
						obj, g_lights, camPos, lp, isSelected
					);
				}
			}
		}

		//9. 显存提交与最终绘制
		SDL_UpdateTexture(tex, NULL, g_frameBuffer.data(), WIDTH * 4);
		SDL_RenderClear(renderer);
		SDL_RenderTexture(renderer, tex, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	// 清理资源
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}