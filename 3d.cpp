#include "lib/frame.h"
#include "lib/game.h"
#include "lib/render.h"
#include <vector>
#include <math.h>
#include <algorithm>

class Vector3D
{
    public:
    float x, y, z;
};

class Triangle
{
    public:
    Vector3D p1, p2, p3;
    char character;
};

class Mesh
{
    public:
    std::vector<Triangle> triangles;
};

class Matrix4x4
{
    public:
    float m[4][4] = { 0};

    void Multiply(Vector3D& input, Vector3D& output)
    {
        output.x = input.x * m[0][0] + input.y * m[1][0] + input.z * m[2][0] + m[3][0];
        output.y = input.x * m[0][1] + input.y * m[1][1] + input.z * m[2][1] + m[3][1];
        output.z = input.x * m[0][2] + input.y * m[1][2] + input.z * m[2][2] + m[3][2];
        float w = input.x * m[0][3] + input.y * m[1][3] + input.z * m[2][3] + m[3][3];
        if (w != 0.0f)
        {
            output.x /= w;
            output.y /= w;
            output.z /= w;
        }
    }
};

class Render3D
{
    private:
    Matrix4x4 projection;
    float fTheta = 0;
    Vector3D vCamera;

    char GetCharacter(float lum)
    {
        int pixel_bw = (int)(13.0f*lum);
        switch (pixel_bw)
        {
            case 1: return '.';
            case 2: 
            case 3: return '-';
            case 4: 
            case 5: return '+';
            case 6: 
            case 7:
            case 8: return '=';
            case 9:
            case 10:
            case 11: return '#'; 
            case 12:
            case 13: return '@';
            default: return ' ';
        };
    }

    public:
    Render3D()
    {
        float fNear = 0.1f;
        float fFar = 1000.0f;
        float fieldOfView = 90.0f;
        float aspectRatio = (float)grid.GetHeight() / (float)grid.GetWidth();
        float fovRad = 1.0f / tanf(fieldOfView * 0.5f / 180.0f * 3.14159f);

        projection.m[0][0] = aspectRatio * fovRad;
        projection.m[1][1] = fovRad;
        projection.m[2][2] = fFar / (fFar - fNear);
        projection.m[3][2] = (-fFar * fNear) / (fFar - fNear);
        projection.m[2][3] = 1.0f;
        projection.m[3][3] = 0.0f;
    }

    void RenderToGrid(Mesh& mesh)
    {
        grid.DrawRectangle(0, 0, grid.GetWidth()-1, grid.GetHeight()-1, ' ');

        Matrix4x4 matRotZ, matRotX;
		fTheta += 0.05f;

        // Rotation Z
		matRotZ.m[0][0] = cosf(fTheta);
		matRotZ.m[0][1] = sinf(fTheta);
		matRotZ.m[1][0] = -sinf(fTheta);
		matRotZ.m[1][1] = cosf(fTheta);
		matRotZ.m[2][2] = 1;
		matRotZ.m[3][3] = 1;

		// Rotation X
		matRotX.m[0][0] = 1;
		matRotX.m[1][1] = cosf(fTheta * 0.5f);
		matRotX.m[1][2] = sinf(fTheta * 0.5f);
		matRotX.m[2][1] = -sinf(fTheta * 0.5f);
		matRotX.m[2][2] = cosf(fTheta * 0.5f);
		matRotX.m[3][3] = 1;

        vector<Triangle> vecTrianglesToRaster;

        for (auto tri : mesh.triangles)
        {
            Triangle triRotatedZ, triRotatedZX;
            // Rotate in Z-Axis
			matRotZ.Multiply(tri.p1, triRotatedZ.p1);
			matRotZ.Multiply(tri.p2, triRotatedZ.p2);
			matRotZ.Multiply(tri.p3, triRotatedZ.p3);

			// Rotate in X-Axis
			matRotX.Multiply(triRotatedZ.p1, triRotatedZX.p1);
			matRotX.Multiply(triRotatedZ.p2, triRotatedZX.p2);
			matRotX.Multiply(triRotatedZ.p3, triRotatedZX.p3);

            Triangle translated = triRotatedZX;
            translated.p1.z = tri.p1.z + 4.0f;
            translated.p2.z = tri.p2.z + 4.0f;
            translated.p3.z = tri.p3.z + 4.0f;

            // Use Cross-Product to get surface normal
			Vector3D normal, line1, line2;
			line1.x = translated.p2.x - translated.p1.x;
			line1.y = translated.p2.y - translated.p1.y;
			line1.z = translated.p2.z - translated.p1.z;

			line2.x = translated.p3.x - translated.p1.x;
			line2.y = translated.p3.y - translated.p1.y;
			line2.z = translated.p3.z - translated.p1.z;

			normal.x = line1.y * line2.z - line1.z * line2.y;
			normal.y = line1.z * line2.x - line1.x * line2.z;
			normal.z = line1.x * line2.y - line1.y * line2.x;

			// It's normally normal to normalise the normal
			float l = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
			normal.x /= l; normal.y /= l; normal.z /= l;

			if(normal.x * (translated.p1.x - vCamera.x) + 
                normal.y * (translated.p1.y - vCamera.y) +
                normal.z * (translated.p1.z - vCamera.z) < 0.0f)
            {
                Vector3D light_direction = { 0.0f, 0.0f, -1.0f };
				float l = sqrtf(light_direction.x*light_direction.x + light_direction.y*light_direction.y + light_direction.z*light_direction.z);
				light_direction.x /= l; light_direction.y /= l; light_direction.z /= l;

				// How similar is normal to light direction
				float dp = normal.x * light_direction.x + normal.y * light_direction.y + normal.z * light_direction.z;

                Triangle projected;
                projection.Multiply(translated.p1, projected.p1);
                projection.Multiply(translated.p2, projected.p2);
                projection.Multiply(translated.p3, projected.p3);

                projected.p1.x += 1.0f; projected.p1.y += 1.0f;
                projected.p2.x += 1.0f; projected.p2.y += 1.0f;
                projected.p3.x += 1.0f; projected.p3.y += 1.0f;

                float width = grid.GetWidth();
                float height = grid.GetHeight();

                projected.p1.x *= width * 0.5;
                projected.p1.y *= height * 0.5;
                projected.p2.x *= width * 0.5;
                projected.p2.y *= height * 0.5;
                projected.p3.x *= width * 0.5;
                projected.p3.y *= height * 0.5;
                projected.character = GetCharacter(dp);

                vecTrianglesToRaster.push_back(projected);
            }

            sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](Triangle &t1, Triangle &t2)
            {
                float z1 = (t1.p1.z + t1.p2.z + t1.p3.z) / 3.0f;
                float z2 = (t2.p1.z + t2.p2.z + t2.p3.z) / 3.0f;
                return z1 > z2;
            });

            for (auto &triProjected : vecTrianglesToRaster)
            {
                // Rasterize triangle
                grid.FillTriangle(
                    triProjected.p1.x, triProjected.p1.y,
                    triProjected.p2.x, triProjected.p2.y,
                    triProjected.p3.x, triProjected.p3.y,
                    triProjected.character);
            }
        }
    }
};

class Game3D : public Game
{
    private:
    Mesh cube;
    Render3D render3D;

    public:
    Game3D(int fps) : Game(fps) 
    {
        cube.triangles = {
            // front
            {0, 0, 0, 0, 1 ,0, 1, 1, 0},
            {0, 0, 0, 1, 1 ,0, 1, 0, 0},

            // right
            {1, 0, 0, 1, 1 ,0, 1, 1, 1},
            {1, 0, 0, 1, 1 ,1, 1, 0, 1},

            // back
            {1, 0, 1, 1, 1 ,1, 0, 1, 1},
            {1, 0, 1, 0, 1 ,1, 0, 0, 1},

            // left
            {0, 0, 1, 0, 1 ,1, 0, 1, 0},
            {0, 0, 1, 0, 1 ,0, 0, 0, 0},

            // top
            {0, 1, 0, 0, 1 ,1, 1, 1, 1},
            {0, 1, 0, 1, 1 ,1, 1, 1, 0},

            // bottom
            {1, 0, 1, 0, 0 ,1, 0, 0, 0},
            {1, 0, 1, 0, 0 ,0, 1, 0, 0}
        };
    }

    void Update() override
    {
        render3D.RenderToGrid(cube);
    }
};

int main(void)
{
    const int FPS{30};
    Game3D r3d{FPS};
    r3d.Start();
    return 0;
}