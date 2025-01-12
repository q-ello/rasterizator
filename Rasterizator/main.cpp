#define NOMINMAX

#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"

Model* model;
const int width = 800;
const int height = 800;

Vec3f light_dir{ 1, 1, 1 };
Vec3f camera{-1, 1, 2};
const Vec3f center{ 0, 0, 0 };

struct MyShader : public IShader {
	std::vector<Vec3f> varying_normal{ {0., 0., 0.},{0., 0., 0.}, {0., 0., 0.} };
	std::vector<Vec3f> varying_uv{ {0., 0., 0.},{0., 0., 0.} };
	mat4 uniform;
	mat4 uniform_it;

	virtual Vec4f vertex(int iface, int nthvert, Model* model) {
		std::vector<std::vector<int>> face = model->face(iface);
		if (!model->hasNormalMap())
		{
			Vec3f normal = model->normal(face[nthvert][2]);
			switch (nthvert)
			{
			case 0:
				varying_normal[0].x = normal.x;
				varying_normal[1].x = normal.y;
				varying_normal[2].x = normal.z;
				break;
			case 1:
				varying_normal[0].y = normal.x;
				varying_normal[1].y = normal.y;
				varying_normal[2].y = normal.z;
				break;
			default:
				varying_normal[0].z = normal.x;
				varying_normal[1].z = normal.y;
				varying_normal[2].z = normal.z;
			}
		}

		Vec2i uv = model->uv(face[nthvert][1]);
		switch (nthvert)
		{
		case 0:
			varying_uv[0].x = uv.x;
			varying_uv[1].x = uv.y;
			break;
		case 1:
			varying_uv[0].y = uv.x;
			varying_uv[1].y = uv.y;
			break;
		default:
			varying_uv[0].z = uv.x;
			varying_uv[1].z = uv.y;
		}

		Vec4f gl_Vertex = Vec4f(model->vert(face[nthvert][0]), 1.); // read the vertex from .obj file
		mat4 transform = Viewport * uniform;

		return transform * gl_Vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor& color, Model* model) {
		float diff;
		Vec2f uv = Vec2f(varying_uv[0].Dot(bar), varying_uv[1].Dot(bar));
		Vec3f n;
		if (model->hasNormalMap())
		{
			n = (uniform_it * Vec4f(model->normal(uv), 1.)).xyz();
			n.Normalize();
			Vec3f l = (uniform * Vec4f(light_dir, 1.)).xyz();
			l.Normalize();
			diff = n.Dot(l);
		}
		else
		{
			n = Vec3f(varying_normal[0].Dot(bar), varying_normal[1].Dot(bar), varying_normal[2].Dot(bar));
			diff = n.Dot(light_dir);
		}
		n.Normalize();
		diff = std::max(0.f, diff);

		Vec3f v = center - camera;
		v.Normalize();
		Vec3f r = Vec3f::Reflect(light_dir, n);
		r.Normalize();

		float spec = pow(std::max(r.Dot(v), 0.f), model->spec(uv));
		float coeff = diff + 1. * spec;

		{
			color.r = std::min(5 + model->diffuse(uv).r * coeff, 255.f);
			color.g = std::min(5 + model->diffuse(uv).g * coeff, 255.f);
			color.b = std::min(5 + model->diffuse(uv).b * coeff, 255.f);
		}
		return false;                              
	}
};

int main(int argc, char** argv) {
	if (argc == 2)
	{
		model = new Model(argv[1]);
	}
	else
	{
		model = new Model("african_head.obj");
	}

	Model* model2 = new Model("Cat.obj");

	TGAImage image(width, height, TGAImage::RGB);

	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
	Vec3f coeff = camera - center; 
	projection(-1. / coeff.Length());
	light_dir.Normalize();

	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	lookat(camera, center, { 0, 1, 0 });

	MyShader shader;
	shader.uniform = Projection * ModelView;
	shader.uniform_it = (Projection * ModelView).Invert().Transpose();
	for (int i = 0; i < model->nfaces(); i++) {
		Vec4f screen_coords[3];
		for (int j = 0; j < 3; j++) {
			screen_coords[j] = shader.vertex(i, j, model);
		}
		triangle(screen_coords, shader, image, zbuffer, model);
	}

	for (int i = 0; i < model2->nfaces(); i++) {
		Vec4f screen_coords[3];
		for (int j = 0; j < 3; j++) {
			screen_coords[j] = shader.vertex(i, j, model2);
		}
		triangle(screen_coords, shader, image, zbuffer, model2);
	}

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	zbuffer.flip_vertically();
	if (argc == 2)
	{
		std::string name = std::string(argv[1]);
		image.write_tga_file((name + ".tga").c_str());
		zbuffer.write_tga_file((name + ".zbuffer.tga").c_str());
	}
	else
	{
		image.write_tga_file("output.tga");
		zbuffer.write_tga_file("zbuffer.tga");
	}
	
	delete model2;
	return 0;
}
