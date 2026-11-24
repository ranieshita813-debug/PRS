#include "TestFramework.h"
#include "PRS/Assets/AssetManager.h"
#include "PRS/Assets/ImageLoader.h"
#include "PRS/Assets/ObjLoader.h"
#include <cstring>
#include <filesystem>
#include <fstream>

using namespace PRS;

namespace
{
ConstSpan<u8> AsBytes(const char* text)
{
    return ConstSpan<u8>(reinterpret_cast<const u8*>(text), std::strlen(text));
}

} // namespace

TEST(OBJ_ParsesIndexFormsAndAttributes)
{
    const char* source = "# triangle\n"
                         "v 0 0 0\n"
                         "v 1 0 0\n"
                         "v 0 1 0\n"
                         "vt 0 0\n"
                         "vt 1 0\n"
                         "vt 0 1\n"
                         "vn 0 0 1\n"
                         "f 1/1/1 2/2/1 3/3/1\n";

    Assets::MeshData mesh;
    String error;
    CHECK(Assets::LoadOBJ(AsBytes(source), "triangle", mesh, error));
    CHECK(mesh.parts.size() == 1);
    const Assets::MeshPart& part = mesh.parts[0];
    CHECK(part.vertices.size() == 3);
    CHECK(part.indices.size() == 3);
    CHECK(NearlyEqual(part.vertices[1].position.x, 1.0f));
    CHECK(NearlyEqual(part.vertices[2].uv.y, 1.0f));
    CHECK(NearlyEqual(part.vertices[0].normal.z, 1.0f));
    CHECK(mesh.TriangleCount() == 1);
}

TEST(OBJ_TriangulatesQuadsAndResolvesNegativeIndices)
{
    const char* source = "v 0 0 0\n"
                         "v 1 0 0\n"
                         "v 1 1 0\n"
                         "v 0 1 0\n"
                         "f -4 -3 -2 -1\n";

    Assets::MeshData mesh;
    String error;
    CHECK(Assets::LoadOBJ(AsBytes(source), "quad", mesh, error));
    CHECK(mesh.parts.size() == 1);
    CHECK(mesh.parts[0].vertices.size() == 4);
    CHECK(mesh.parts[0].indices.size() == 6); // quad -> two triangles
    // No vn given, so smooth normals are generated pointing along +Z.
    for (const Assets::MeshVertex& vertex : mesh.parts[0].vertices)
    {
        CHECK(NearlyEqual(Length(vertex.normal), 1.0f));
        CHECK(NearlyEqual(vertex.normal.z, 1.0f));
    }
}

TEST(OBJ_SplitsPartsPerMaterial)
{
    const char* source = "v 0 0 0\n"
                         "v 1 0 0\n"
                         "v 0 1 0\n"
                         "usemtl red\n"
                         "f 1 2 3\n"
                         "usemtl blue\n"
                         "f 1 2 3\n";

    Assets::MeshData mesh;
    String error;
    CHECK(Assets::LoadOBJ(AsBytes(source), "two", mesh, error));
    CHECK(mesh.parts.size() == 2);
    CHECK(mesh.parts[0].material.name == "red");
    CHECK(mesh.parts[1].material.name == "blue");
    CHECK(mesh.IndexCount() == 6);
}

TEST(OBJ_RejectsFilesWithoutFaces)
{
    Assets::MeshData mesh;
    String error;
    CHECK(!Assets::LoadOBJ(AsBytes("v 0 0 0\n"), "empty", mesh, error));
    CHECK(!error.empty());
}

TEST(MTL_ParsesMaterialProperties)
{
    const char* source = "newmtl brick\n"
                         "Kd 0.8 0.4 0.2\n"
                         "Ks 0.5 0.5 0.5\n"
                         "Ns 32\n"
                         "d 0.75\n"
                         "map_Kd Textures/brick.tga\n";

    HashMap<String, Assets::MaterialData> materials;
    String error;
    CHECK(Assets::LoadMTL(AsBytes(source), materials, error));
    CHECK(materials.size() == 1);
    const Assets::MaterialData& brick = materials["brick"];
    CHECK(NearlyEqual(brick.baseColor.x, 0.8f));
    CHECK(NearlyEqual(brick.baseColor.y, 0.4f));
    CHECK(NearlyEqual(brick.baseColor.w, 0.75f));
    CHECK(NearlyEqual(brick.shininess, 32.0f));
    CHECK(brick.baseColorTexture == "Textures/brick.tga");
    CHECK(brick.HasTexture());
}

TEST(TGA_DecodesUncompressedAndFlipsRows)
{
    // 2x2, 24-bit, bottom-up (descriptor 0).
    std::vector<u8> file = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 24, 0};
    // Bottom row first: (r=10), (r=20); then top row: (r=30), (r=40).
    const u8 rows[12] = {0, 0, 10, 0, 0, 20, 0, 0, 30, 0, 0, 40};
    file.insert(file.end(), rows, rows + 12);

    Assets::Image image;
    String error;
    CHECK(Assets::LoadTGA(ConstSpan<u8>(file.data(), file.size()), image, error));
    CHECK(image.width == 2 && image.height == 2);
    CHECK(image.IsValid());
    const u8* topLeft = image.Texel(0, 0);
    const u8* bottomRight = image.Texel(1, 1);
    CHECK(topLeft != nullptr && topLeft[0] == 30);
    CHECK(topLeft[3] == 255);
    CHECK(bottomRight != nullptr && bottomRight[0] == 20);
}

TEST(TGA_DecodesRleRuns)
{
    // 2x1, 32-bit, top-down, one RLE packet of two identical texels.
    std::vector<u8> file = {0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 32, 0x20 | 8};
    const u8 packet[5] = {0x81, 255, 0, 0, 255}; // run of 2, BGRA blue
    file.insert(file.end(), packet, packet + 5);

    Assets::Image image;
    String error;
    CHECK(Assets::LoadTGA(ConstSpan<u8>(file.data(), file.size()), image, error));
    CHECK(image.width == 2 && image.height == 1);
    const u8* left = image.Texel(0, 0);
    const u8* right = image.Texel(1, 0);
    CHECK(left != nullptr && left[2] == 255 && left[0] == 0);
    CHECK(right != nullptr && right[2] == 255);
}

TEST(BMP_DecodesBottomUpWithPadding)
{
    // 2x2, 24-bit. Stride is 8 bytes (6 data + 2 padding).
    std::vector<u8> file;
    file.resize(54, 0);
    file[0] = 'B';
    file[1] = 'M';
    const u32 pixelOffset = 54;
    std::memcpy(&file[10], &pixelOffset, 4);
    const u32 dibSize = 40;
    std::memcpy(&file[14], &dibSize, 4);
    const i32 width = 2;
    const i32 height = 2;
    std::memcpy(&file[18], &width, 4);
    std::memcpy(&file[22], &height, 4);
    const u16 planes = 1;
    std::memcpy(&file[26], &planes, 2);
    const u16 bpp = 24;
    std::memcpy(&file[28], &bpp, 2);
    // Bottom row then top row, each with 2 bytes of padding.
    const u8 pixels[16] = {0, 0, 50, 0, 0, 60, 0, 0, 0, 0, 70, 0, 0, 80, 0, 0};
    file.insert(file.end(), pixels, pixels + 16);

    Assets::Image image;
    String error;
    CHECK(Assets::LoadBMP(ConstSpan<u8>(file.data(), file.size()), image, error));
    CHECK(image.width == 2 && image.height == 2);
    const u8* topLeft = image.Texel(0, 0);
    const u8* bottomRight = image.Texel(1, 1);
    CHECK(topLeft != nullptr && topLeft[0] == 70);
    CHECK(bottomRight != nullptr && bottomRight[0] == 60);
}

TEST(Image_RejectsUnknownExtensions)
{
    std::vector<u8> bytes = {1, 2, 3};
    Assets::Image image;
    String error;
    CHECK(!Assets::LoadImage(ConstSpan<u8>(bytes.data(), bytes.size()), "photo.png", image, error));
    CHECK(error.find("png") != String::npos);
}

TEST(AssetManager_ResolvesAndCachesMeshes)
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "prs_asset_tests" / "assets";
    std::error_code ec;
    std::filesystem::create_directories(root / "Models", ec);

    const std::filesystem::path objPath = root / "Models" / "triangle.obj";
    std::ofstream obj(objPath);
    obj << "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    obj.close();

    Assets::AssetManager assets;
    assets.AddSearchPath((root.parent_path() / "missing").string());
    assets.AddSearchPath(root.string());

    CHECK(assets.Exists("Models/triangle.obj"));
    CHECK(!assets.Exists("Models/nope.obj"));

    const Assets::MeshData* mesh = assets.LoadMesh("Models/triangle.obj");
    CHECK(mesh != nullptr);
    CHECK(mesh != nullptr && mesh->TriangleCount() == 1);
    CHECK(assets.LoadMesh("Models/triangle.obj") == mesh); // cached, not re-read
    CHECK(assets.MeshCount() == 1);
    CHECK(assets.LoadMesh("Models/nope.obj") == nullptr);

    std::filesystem::remove_all(std::filesystem::temp_directory_path() / "prs_asset_tests", ec);
}
