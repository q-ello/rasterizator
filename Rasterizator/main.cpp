#include "tgaimage.h"
#include "model.h"
#include <limits>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

//initialized operators and constructors
Vec2i operator -(const Vec2i& a, const Vec2i& b)
{
	return { a.x - b.x, a.y - b.y };
}

Vec2i operator +(const Vec2i& a, const Vec2f& b)
{
	return { a.x + (int)(b.x + .5), a.y + (int)(b.y + .5) };
}

Vec3i operator -(const Vec3i& a, const Vec3i& b)
{
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3i operator +(const Vec3i& a, const Vec3f& b)
{
	return { a.x + (int)(b.x + .5), a.y + (int)(b.y + .5), a.z + (int)(b.z + .5)};
}



Model* model;
const int width = 800;
const int height = 800;
const int depth = 255;

const Vec3f light_dir{ 0, 0, 1 };

void line(Vec2i v0, Vec2i v1, TGAImage& image, TGAColor color) {
	bool steep = false;
	int x0 = v0.x;
	int x1 = v1.x;
	int y0 = v0.y;
	int y1 = v1.y;

	if (std::abs(x1 - x0) < std::abs(y1 - y0))
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dx = x1 - x0;
	int dy = y1 - y0;
	int derror = std::abs(2*dy);
	int error = 0;
	int y = y0;
	for (int x = x0; x <= x1; x++)
	{
		if (steep)
		{
			image.set(y, x, color);
		}
		else
		{
			image.set(x, y, color);
		}
		error += derror;
		if (error >= dx)
		{
			y += y1>y0?1:-1;
			error -= 2*dx;
		}
	}
}

Vec3f barycentric(Vec3i P, Vec3i bounds[3])
{
	Vec3i vx = bounds[1] - bounds[0];
	Vec3i vy = bounds[2] - bounds[0];
	Vec3i vz = bounds[0] - P;
	Vec3f u = Vec3f(vx.x, vy.x, vz.x).Cross(Vec3f(vx.y, vy.y, vz.y));
	if (std::abs(u.z) < 1) return { -1, 1, 1 };
	u /= u.z;
	return { 1 - u.x - u.y, u.x, u.y };
}

void triangle(Vec3i t[3], TGAImage& image, TGAColor color) {
	//finding bounding box
	int minX = width;
	int minY = height;
	int maxX = 0;
	int maxY = 0;
	for (int i = 0; i < 3; i++)
	{
		minX = minX < t[i].x ? minX : t[i].x;
		minY = minY < t[i].y ? minY : t[i].y;
		maxX = maxX > t[i].x ? maxX : t[i].x;
		maxY = maxY > t[i].y ? maxY : t[i].y;
	}
	if (minX < 0) minX = 0;
	if (minY < 0) minY = 0;
	if (maxX > width) maxX = width;
	if (maxY > height) maxY = height;

	for (int x = minX; x <= maxX; x++)
		for (int y = minY; y <= maxY; y++)
		{
			Vec3f bc_screen = barycentric({ x, y, 0 }, t);
			int idx = x + y * width;
			Vec3f bc_clip = Vec3f(bc_screen.x / t[0].z, bc_screen.y / t[1].z, bc_screen.z / t[2].z);
			bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0 )//|| zbuffer[idx] > bc_screen.z)
			{
				continue;
			}
			//zbuffer[idx] = bc_screen.z;
			image.set(x, y, color);
		}
}

void triangle(Vec3i t[3], Vec2i uv[3], TGAImage& image, float intensity, int* zbuffer) {
	if (t[0].y == t[1].y && t[0].y == t[2].y) return; // i dont care about degenerate triangles
	if (t[0].y > t[1].y)
	{
		std::swap(t[0], t[1]);
		std::swap(uv[0], uv[1]);
	}
	if (t[0].y > t[2].y)
	{
		std::swap(t[0], t[2]);
		std::swap(uv[0], uv[2]);
	}
	if (t[1].y > t[2].y)
	{
		std::swap(t[1], t[2]);
		std::swap(uv[1], uv[2]);
	}
	int total_height = t[2].y - t[0].y;
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t[1].y - t[0].y || t[1].y == t[0].y;
		int segment_height = second_half ? t[2].y - t[1].y : t[1].y - t[0].y;
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t[1].y - t[0].y : 0)) / segment_height; // be careful: with above conditions no division by zero here
		Vec3i A = t[0] + Vec3f(t[2] - t[0]) * alpha;
		Vec3i B = second_half ? t[1] + Vec3f(t[2] - t[1]) * beta : t[0] + Vec3f(t[1] - t[0]) * beta;

		Vec2i uvA = uv[0] + Vec2f(uv[2] - uv[0]) * alpha;
		Vec2i uvB = second_half ? uv[1] + Vec2f(uv[2] - uv[1]) * beta : uv[0] + Vec2f(uv[1] - uv[0]) * beta;
		if (A.x > B.x)
		{
			std::swap(A, B);
			std::swap(uvA, uvB);
		}
		for (int j = A.x; j <= B.x; j++) {
			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
			Vec3f Pf = Vec3f(A) + Vec3f(B - A) * phi;
			Vec3i P{(int)(Pf.x + .5), (int)(Pf.y + .5), (int)(Pf.z + .5)};
			

			int idx = P.x + P.y * width;
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;

				Vec2f uvPf = Vec2f(uvA) + Vec2f(uvB - uvA) * phi;
				Vec2i uvP{ (int)(uvPf.x + .5), (int)(uvPf.y + .5) };
				TGAColor diffColor = model->diffuse(uvP);

				image.set(P.x, P.y, diffColor * intensity);
			}
		}
	}
}

int main(int argc, char** argv) {
	if (argc == 2)
	{
		model = new Model(argv[1]);
	}
	else
	{
		model = new Model("obj/african_head.obj");
	}

	TGAImage image(width, height, TGAImage::RGB);

	int *zbuffer = new int[width * height];

	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<std::vector<int>> face = model->face(i);
		Vec3i screen_coords[3];
		Vec3f world_coords[3];
		Vec2i uv_coords[3];
		for (int j = 0; j < 3; j++) {
			Vec3f v = model->vert(face[0][j]);
			screen_coords[j] = Vec3i((v.x + 1.) * width / 2., (v.y + 1.) * height / 2., (v.z + 1.) * depth / 2.);
			uv_coords[j] = model->uv(face[1][j]);
			world_coords[j] = v;
		}
		Vec3f normal = (world_coords[1] - world_coords[0]).Cross(world_coords[2] - world_coords[0]);
		normal.Normalize();
		float intensity = light_dir.Dot(normal);
		if (intensity > 0)
		{
			triangle(screen_coords, uv_coords, image, intensity, zbuffer);
		}
	}

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	return 0;
}
