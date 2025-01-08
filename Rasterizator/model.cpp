#include "model.h"

Model::Model(std::string filename)
{
	std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error opening the file!";
        return;
    }

    std::string line;

    while (!file.eof())
    {
        std::getline(file, line);
        std::stringstream ss(line);
        std::string word;
        while (ss >> word)
        {
            if (word == "v")
            {
                Vec3f vertix;
                ss >> vertix.x >> vertix.y >> vertix.z;
                _vertices.push_back(vertix);
            }
            else if (word == "f")
            {
                std::vector<std::vector<int>> face{ {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
                for (int i = 0; i < 3; i++)
                {
                    ss >> word;
                    std::stringstream iss(word);
                    std::string vertix;
                    for (int j = 0; j < 3; j++)
                    {
                        std::getline(iss, vertix, '/');
                        face[j][i] = std::stoi(vertix);
                    }
                }
                _faces.push_back(face);
            }
            else if (word == "vt")
            {
                Vec2f texVertix;
                ss >> texVertix.x >> texVertix.y;
                _texVertices.push_back(texVertix);
            }
            else
            {
                break;
            }
        }
    }

    std::cout << "file has been read: " << _vertices.size() << " vertices, " << _faces.size() << " faces." << std::endl;
    load_texture(filename, "_diffuse.tga", _diffMap);

    file.close();
}

int Model::nfaces()
{
    return _faces.size();
}

std::vector<std::vector<int>> Model::face(int i)
{
    return _faces[i];
}

Vec3f Model::vert(int i)
{
    return _vertices[i];
}

Vec2i Model::uv(int i)
{
    Vec2f vertix = _texVertices[i];
    return { int(vertix.x*_diffMap.get_width() + .5), int(vertix.y * _diffMap.get_height() + .5)};
}

TGAColor Model::diffuse(Vec2i uv)
{
    return _diffMap.get(uv.x, uv.y);
}

void Model::load_texture(std::string filename, const char* suffix, TGAImage& img)
{
    std::string texfile(filename);
    size_t dot = texfile.find_last_of(".");
    if (dot != std::string::npos) {
        texfile = texfile.substr(0, dot) + std::string(suffix);
        std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        img.flip_vertically();
    }
}