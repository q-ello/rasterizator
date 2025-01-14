#define NOMINMAX

#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"

Model* model;
std::vector<Model*> models;
int width = 800;
int height = 800;

Vec3f light_dir{ 1, 1, 1 };
Vec3f camera{0, 0, 3};
Vec3f center{ 0, 0, 0 };

std::string message;

std::string outputName{};


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
			varying_uv[0].x = (float)uv.x;
			varying_uv[1].x = (float)uv.y;
			break;
		case 1:
			varying_uv[0].y = (float)uv.x;
			varying_uv[1].y = (float)uv.y;
			break;
		default:
			varying_uv[0].z = (float)uv.x;
			varying_uv[1].z = (float)uv.y;
		}

		Vec4f gl_Vertex = Vec4f(model->vert(face[nthvert][0]), 1.); // read the vertex from .obj file
		mat4 transform = Viewport * uniform;

		return Vec4f::Transform(gl_Vertex, transform.Transpose());
	}

	virtual bool fragment(Vec3f bar, TGAColor& color, Model* model) {
		float diff;
		Vec2f uv = Vec2f(varying_uv[0].Dot(bar), varying_uv[1].Dot(bar));
		Vec3f n;
		if (model->hasNormalMap())
		{
			n = Vec4f::Transform(Vec4f(model->normal(uv), 1.), uniform_it.Transpose()).xyz();
			n.Normalize();
			Vec3f l = Vec4f::Transform(Vec4f(light_dir, 1.), uniform.Transpose()).xyz();
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
		float coeff = diff + 1.f * spec;

		{
			color.r = std::min(5 + model->diffuse(uv).r * coeff, 255.f);
			color.g = std::min(5 + model->diffuse(uv).g * coeff, 255.f);
			color.b = std::min(5 + model->diffuse(uv).b * coeff, 255.f);
		}
		return false;                              
	}
};

bool processCommand(const std::string& command)
{
	std::stringstream c{ command };
	std::string arg;
	std::vector<std::string> args{};
	while (c >> arg)
	{
		args.push_back(arg);
	}
	if (args[0] == "render" && args.size() == 1)
	{
		return true;
	}
	if (args[0] == "light" && args.size() == 4)
	{
		Vec3f newLD;
		try
		{
			newLD.x = std::stof(args[1]);
			newLD.y = std::stof(args[2]);
			newLD.z = std::stof(args[3]);
			light_dir = newLD;
		}
		catch (...)
		{
			message = "Invalid arguments for light: cannot convert to float";
		}
	}
	else if (args[0] == "resize" && args.size() == 3)
	{
		Vec2i newSize;
		try
		{
			newSize.x = std::stoi(args[1]);
			newSize.y = std::stoi(args[2]);
			width = newSize.x;
			height = newSize.y;
		}
		catch (...)
		{
			message = "Invalid arguments for resize: cannot convert to int";
		}
	}
	else if (args[0] == "camera" && args.size() == 4)
	{
		Vec3f newCamera;
		try
		{
			newCamera.x = std::stof(args[1]);
			newCamera.y = std::stof(args[2]);
			newCamera.z = std::stof(args[3]);
			camera = newCamera;
		}
		catch (...)
		{
			message = "Invalid arguments for camera: cannot convert to float";
		}
	}
	else if (args[0] == "center" && args.size() == 4)
	{
		Vec3f newCenter;
		try
		{
			newCenter.x = std::stof(args[1]);
			newCenter.y = std::stof(args[2]);
			newCenter.z = std::stof(args[3]);
			center = newCenter;
		}
		catch (...)
		{
			message = "Invalid arguments for center: cannot convert to float";
		}
	}
	else if (args[0] == "add_model" && args.size() == 2)
	{
		Vec3f modelCenter;
		float specCoeff;
		try
		{
			std::string position;
			std::vector<std::string> buffer;
			std::cout << "Write model position in format 'float float float'" << std::endl;
			std::getline(std::cin, position);
			std::stringstream ss{ position };
			while (ss >> position)
			{
				buffer.push_back(position);
			}
			modelCenter.x = std::stof(buffer[0]);
			modelCenter.y = std::stof(buffer[1]);
			modelCenter.z = std::stof(buffer[2]);
			float specCoeff;

			std::cout << "Write float specular coefficient from 0 to 100" << std::endl;
			std::getline(std::cin, position);
			specCoeff = std::stof(position);

			Model* newModel = new Model(args[1], modelCenter, specCoeff);
			if (newModel->nfaces() > 0)
			{
				models.push_back(std::move(newModel));
				message = "Model was successfully added to queue for rendering";
				outputName += (models.size() > 1 ? "_and_" : "") + args[1].substr(0, args[1].length() - 4);
			}
			else
			{
				message = "Could not add model to queue: filename could be wrong ";
			}
		}
		catch (...)
		{
			message = "Invalid arguments for adding model";
		}
	}
	else
	{
		message = "Could not recognize command";
	}
	return false;
}

int main(int argc, char** argv) {
	if (argc == 2)
	{
		models.push_back(new Model(argv[1]));
	}
	else
	{ 
		while (true)
		{
			std::cout << "Hello! This rasterizator will render your .obj files, but I need to put some disclaimers first:" << std::endl;
			std::cout << "This program will render only triangles, so if your object has quad faces, then the object will only be half-rendered.\n";
			std::cout << "This program can work with such textures as normalmap, diffusemap and specularmap. ";
			std::cout << "To work, it needs to have them in the same location as.obj file in .tga format and be named as 'objname_nm.tga, 'objname_diffuse.tga' and 'objname_spec.tga' respectively.\n";
			std::cout << "This is some data you can add or change, just type one of these following commands:\n";
			std::cout << "- 'resize int int' - change width and height of output tga image (current: " << width << ' ' << height << ");\n";
			std::cout << "- 'light float float float' - change light position (current: " << light_dir.x << ' ' << light_dir.y << ' ' << light_dir.z << ");\n";
			std::cout << "- 'camera float float float' - change camera position (current: " << camera.x << ' ' << camera.y << ' ' << camera.z << ");\n";
			std::cout << "- 'center float float float' - change lookat point position (current: " << center.x << ' ' << center.y << ' ' << center.z << ");\n";
			std::cout << "- 'add_model filename.obj' - add another model to render with its position and specular coefficient (models to render: " << models.size() << ");\n";
			std::cout << "- 'render' - that's it, render\n";
			std::cout << std::endl;
			std::cerr << message << std::endl;
			std::string command;
			std::getline(std::cin, command);
			if (processCommand(command))
			{
				break;
			}
			system("cls");

		}
	}

	TGAImage image(width, height, TGAImage::RGB);

	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
	Vec3f coeff = camera - center; 
	projection(-1.f / coeff.Length());
	light_dir.Normalize();

	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	lookat(camera, center, { 0, 1, 0 });

	MyShader shader;
	for (Model* model : models)
	{
		shader.uniform = Projection * View * model->ModelView();
		shader.uniform_it = (Projection * View * model->ModelView()).Invert().Transpose();
		for (int i = 0; i < model->nfaces(); i++) {
			Vec4f screen_coords[3];
			for (int j = 0; j < 3; j++) {
				screen_coords[j] = shader.vertex(i, j, model);
			}
			triangle(screen_coords, shader, image, zbuffer, model);
		}
	}

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	zbuffer.flip_vertically();
	if (argc == 2)
	{
		outputName = std::string(argv[1]);
		outputName = outputName.substr(0, outputName.length() - 4);
	}
	image.write_tga_file((outputName + ".tga").c_str());
	zbuffer.write_tga_file((outputName + ".zbuffer.tga").c_str());

	models.clear();
	
	return 0;
}
