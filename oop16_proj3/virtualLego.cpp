////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: 박창현 Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <d3dx9core.h>

IDirect3DDevice9* Device = NULL;
ID3DXFont* g_pFont = NULL;

// window size
const int Width = 1024;
const int Height = 768;

// 마지막 점수
int lastScore = 1;

int flag3cushion = false;

int turn = 0;
int flag1 = 0;
int flag2 = 0;
int flag3 = 0;//적 공이랑 부딪혔을 때
int spaceCnt = 0;
int spaceCmp = spaceCnt;
// There are four balls
// initialize the position (coordinate) of each ball (ball0 ~ ball3)
const float spherePos[4][2] = { {-2.7f,0} , {+2.4f,0} , {3.3f,0} , {-2.7f,-0.9f} };
// initialize the color of each ball (ball0 ~ ball3)
const D3DXCOLOR sphereColor[4] = { d3d::RED, d3d::RED, d3d::YELLOW, d3d::WHITE };

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982

int num1 = 0;
int num2 = 0;
int exitflag = 0;
class Text {
private:
    std::string playerNum;
public:
    Text(std::string num) {
        playerNum = num;
    }
    void InitFont(IDirect3DDevice9* pDevice) {
        D3DXFONT_DESC fontDesc;
        ZeroMemory(&fontDesc, sizeof(D3DXFONT_DESC));
        fontDesc.Height = 24;  // 글자 크기
        fontDesc.Width = 0;
        fontDesc.Weight = 0;
        fontDesc.MipLevels = 1;
        fontDesc.Italic = false;
        fontDesc.CharSet = DEFAULT_CHARSET;
        fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
        fontDesc.Quality = DEFAULT_QUALITY;
        fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        strcpy_s(fontDesc.FaceName, LF_FACESIZE, "Arial");

        D3DXCreateFontIndirect(pDevice, &fontDesc, &g_pFont);
    }

    // Function to draw text on the screen
    void DrawText(IDirect3DDevice9* pDevice, int x, int y) {
        RECT rect;
        // 문자열 생성
        char buffer[64];
        if (playerNum == "player1") {
            sprintf_s(buffer, sizeof(buffer), "player1: %d", num1);

            // num 표시될 좌표
            int numX = x;
            int numY = y + 30;

            // num을 출력
            SetRect(&rect, numX, numY, 0, 0);
            if (turn == 1) {
                g_pFont->DrawText(NULL, buffer, -1, &rect, DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
            }
            else {
                g_pFont->DrawText(NULL, buffer, -1, &rect, DT_NOCLIP, D3DCOLOR_XRGB(255, 0, 0));
            }
        }
        if (playerNum == "player2") {
            sprintf_s(buffer, sizeof(buffer), "player2: %d", num2);

            // num 표시될 좌표
            int numX = x;
            int numY = y + 30;

            // num을 출력
            SetRect(&rect, numX, numY, 0, 0);
            if (turn == 0) {
                g_pFont->DrawText(NULL, buffer, -1, &rect, DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
            }
            else {
                g_pFont->DrawText(NULL, buffer, -1, &rect, DT_NOCLIP, D3DCOLOR_XRGB(255, 0, 0));
            }
        }
    }

    void RenderText(int x, int y) {
        DrawText(Device, x, y);
    }
};
class Score {
public:
    void InitFont(IDirect3DDevice9* pDevice) {
        D3DXFONT_DESC fontDesc;
        ZeroMemory(&fontDesc, sizeof(D3DXFONT_DESC));
        fontDesc.Height = 50;  // 글자 크기
        fontDesc.Width = 10;
        fontDesc.Weight = 10;
        fontDesc.MipLevels = 1;
        fontDesc.Italic = false;
        fontDesc.CharSet = DEFAULT_CHARSET;
        fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
        fontDesc.Quality = DEFAULT_QUALITY;
        fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        strcpy_s(fontDesc.FaceName, LF_FACESIZE, "Arial");

        D3DXCreateFontIndirect(pDevice, &fontDesc, &g_pFont);
    }

    // Function to draw text on the screen
    void DrawText(IDirect3DDevice9* pDevice, const char* text, int x, int y) {
        RECT rect;
        SetRect(&rect, x, y, 0, 0);
        g_pFont->DrawText(NULL, text, -1, &rect, DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
    }

    void RenderText(const char* text, int x, int y) {
        DrawText(Device, text, x, y);
    }
};
Text player1("player1");
Text player2("player2");
Score score;
// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
    float               center_x, center_y, center_z; //위치
    float                   m_radius; //반지름
    float               m_velocity_x; //x방향 속도
    float               m_velocity_z; //z방향 속도
    int                  howManyHitWall; //벽 몇 번 쳤는지


public:
    CSphere(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_radius = 0;
        m_velocity_x = 0;
        m_velocity_z = 0;
        m_pSphereMesh = NULL;
    }
    ~CSphere(void) {}

public:
    bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;
        //구의 재질
        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        //생성 및 성공 여부
        if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
            return false;
        return true;
    }

    //구 제거
    void destroy(void)
    {
        if (m_pSphereMesh != NULL) {
            m_pSphereMesh->Release();
            m_pSphereMesh = NULL;
        }
    }

    //생성된 구 렌더링
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
        m_pSphereMesh->DrawSubset(0);
    }
    // 두 구 사이에 충돌이 있는지 확인
    bool hasIntersected(CSphere& ball)
    {
        //(중심 점 사이의 거리)^2 == (공들의 반지름 합)^2
        if (sqrt(pow((this->center_x - ball.center_x), 2) + pow((this->center_z) - (ball.center_z), 2)) <= this->getRadius() + ball.getRadius()) {
            return true;
        }
        return false;
    }
    //충돌 시 수행
    void hitBy(CSphere& ball)
    {
        if (this->hasIntersected(ball)) {
            D3DXVECTOR3 ballPos = ball.getCenter(); //부딪히는 공의 위치
            double dx = this->center_x - ballPos.x;
            double dz = this->center_z - ballPos.z;
            double distance = sqrt(pow(dx, 2) + pow(dz, 2)); //사이 거리

            double whiteVx = this->m_velocity_x;
            double whiteVz = this->m_velocity_z;
            double targetVx = ball.m_velocity_x;
            double targetVz = ball.m_velocity_z;

            double whiteV = sqrt(pow(whiteVx, 2) + pow(whiteVz, 2));  //하얀 공 속도

            double cosTheta = dx / distance; //코사인
            double sinTheta = dz / distance; //사인

            double whiteVx2 = targetVx * cosTheta + targetVz * sinTheta;
            double targetVx2 = whiteVx * cosTheta + whiteVz * sinTheta;
            double whiteVz2 = whiteVz * cosTheta - whiteVx * sinTheta;
            double targetVz2 = targetVz * cosTheta - targetVx * sinTheta;

            this->setPower(whiteVx2 * cosTheta - whiteVz2 * sinTheta, whiteVx2 * sinTheta + whiteVz2 * cosTheta);
            ball.setPower(targetVx2 * cosTheta - targetVz2 * sinTheta, targetVx2 * sinTheta + targetVz2 * cosTheta);
        }
        // Insert your code here.
    }
    void addNumOfWallHit() {
        howManyHitWall++;

    }
    int getNumOfWallHitted() {
        return howManyHitWall;
    }
    void setNumOfWallHitted(int num) {
        howManyHitWall = 0;
    }

    void ballUpdate(float timeDiff)
    {
        const float TIME_SCALE = 3.3;
        D3DXVECTOR3 cord = this->getCenter();
        double vx = abs(this->getVelocity_X());
        double vz = abs(this->getVelocity_Z());

        if (vx > 0.01 || vz > 0.01)
        {
            float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
            float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;

            //correction of position of ball
            // Please uncomment this part because this correction of ball position is necessary when a ball collides with a wall
            if (tX >= (4.5 - M_RADIUS))
                tX = 4.5 - M_RADIUS;
            else if (tX <= (-4.5 + M_RADIUS))
                tX = -4.5 + M_RADIUS;
            else if (tZ <= (-3 + M_RADIUS))
                tZ = -3 + M_RADIUS;
            else if (tZ >= (3 - M_RADIUS))
                tZ = 3 - M_RADIUS;

            this->setCenter(tX, cord.y, tZ);
        }
        else { this->setPower(0, 0); }
        //this->setPower(this->getVelocity_X() * DECREASE_RATE, this->getVelocity_Z() * DECREASE_RATE);
        double rate = 1 - (1 - DECREASE_RATE) * timeDiff * 400;
        if (rate < 0)
            rate = 0;
        this->setPower(getVelocity_X() * rate, getVelocity_Z() * rate);
    }

    double getVelocity_X() { return this->m_velocity_x; }
    double getVelocity_Z() { return this->m_velocity_z; }

    void setPower(double vx, double vz)
    {
        this->m_velocity_x = vx;
        this->m_velocity_z = vz;
    }

    void setCenter(float x, float y, float z)
    {
        D3DXMATRIX m;
        center_x = x;   center_y = y;   center_z = z;
        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }

    float getRadius(void)  const { return (float)(M_RADIUS); }
    const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
    D3DXVECTOR3 getCenter(void) const
    {
        D3DXVECTOR3 org(center_x, center_y, center_z);
        return org;
    }

private:
    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh* m_pSphereMesh;

};

CSphere   g_sphere[4];
CSphere   g_target_blueball;


// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

    float               m_x;
    float               m_z;
    float                   m_width;
    float                   m_depth;
    float               m_height;

public:
    CWall(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_width = 0;
        m_depth = 0;
        m_pBoundMesh = NULL;
    }
    ~CWall(void) {}
public:
    /*
    ix, iz : 박스의 위치
    iwidth, iheght, idepth : 박스의 크기
    */
    bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;

        //박스의 재질
        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        m_width = iwidth;
        m_depth = idepth;

        if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
            return false;
        return true;
    }
    //박스 제거
    void destroy(void)
    {
        if (m_pBoundMesh != NULL) {
            m_pBoundMesh->Release();
            m_pBoundMesh = NULL;
        }
    }
    //렌더링
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
        m_pBoundMesh->DrawSubset(0);
    }

    //구와 벽 사이에 충돌 확인
    bool hasIntersected(CSphere& ball)
    {
        D3DXVECTOR3   ballPos = ball.getCenter();

        if (ballPos.x >= 4.28 || ballPos.x <= -4.28 || ballPos.z >= 2.78 || ballPos.z <= -2.78) {
            return true;
        }
        return false;
    }
   
    //충돌 시 작업
    void hitBy(CSphere& ball)
    {


        //마지막 점수
        if ((turn == 0 && num1 == lastScore - 1) || (turn == 1 && num2 == lastScore - 1))flag3cushion = true;

        // Insert your code here.
        if (hasIntersected(ball)) {
            if ((ball.getCenter().x >= 4.28)) {
                ball.setPower(-ball.getVelocity_X(), ball.getVelocity_Z());
                ball.setCenter(4.27, ball.getCenter().y, ball.getCenter().z);
                if (flag3cushion && ((turn == 0 && ball.getCenter() == g_sphere[3].getCenter()) || (turn == 1 && ball.getCenter() == g_sphere[2].getCenter()))) { ball.addNumOfWallHit(); }
            }
            else if (ball.getCenter().x <= -4.28) {
                ball.setPower(-ball.getVelocity_X(), ball.getVelocity_Z());
                ball.setCenter(-4.27, ball.getCenter().y, ball.getCenter().z);
                if (flag3cushion && ((turn == 0 && ball.getCenter() == g_sphere[3].getCenter()) || (turn == 1 && ball.getCenter() == g_sphere[2].getCenter()))) { ball.addNumOfWallHit(); }
            }

            else if (ball.getCenter().z >= 2.78) {
                ball.setPower(ball.getVelocity_X(), -ball.getVelocity_Z());
                ball.setCenter(ball.getCenter().x, ball.getCenter().y, 2.77);
                if (flag3cushion && ((turn == 0 && ball.getCenter() == g_sphere[3].getCenter()) || (turn == 1 && ball.getCenter() == g_sphere[2].getCenter()))) { ball.addNumOfWallHit(); }
            }
            else if (ball.getCenter().z <= -2.78) {
                ball.setPower(ball.getVelocity_X(), -ball.getVelocity_Z());
                ball.setCenter(ball.getCenter().x, ball.getCenter().y, -2.77);
                if (flag3cushion && ((turn == 0 && ball.getCenter() == g_sphere[3].getCenter()) || (turn == 1 && ball.getCenter() == g_sphere[2].getCenter()))) { ball.addNumOfWallHit(); }
            }
        }
        if (ball.getNumOfWallHitted() >= 3) {
            if (exitflag == 0 && flag1 >= 1 && flag2 == 0 && flag3 == 0) {
                exitflag++;
            }
            else if (exitflag == 1 && flag1 >= 1 && flag2 >= 1 && flag3 == 0) {
                exitflag++;
            }
            else if (exitflag == 0 && flag1 == 0 && flag2 >= 1 && flag3 == 0) {
                exitflag++;
            }
            else if (exitflag == 1 && flag1 >= 0 && flag2 >= 1 && flag3 == 0) {
                exitflag++;
            }

        }
    }

    void setPosition(float x, float y, float z)
    {
        D3DXMATRIX m;
        this->m_x = x;
        this->m_z = z;

        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }

    float getHeight(void) const { return M_HEIGHT; }



private:
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh* m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
    CLight(void)
    {
        static DWORD i = 0;
        m_index = i++;
        D3DXMatrixIdentity(&m_mLocal);
        ::ZeroMemory(&m_lit, sizeof(m_lit));
        m_pMesh = NULL;
        m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        m_bound._radius = 0.0f;
    }
    ~CLight(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
    {
        if (NULL == pDevice)
            return false;
        if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
            return false;

        m_bound._center = lit.Position;
        m_bound._radius = radius;

        m_lit.Type = lit.Type;
        m_lit.Diffuse = lit.Diffuse;
        m_lit.Specular = lit.Specular;
        m_lit.Ambient = lit.Ambient;
        m_lit.Position = lit.Position;
        m_lit.Direction = lit.Direction;
        m_lit.Range = lit.Range;
        m_lit.Falloff = lit.Falloff;
        m_lit.Attenuation0 = lit.Attenuation0;
        m_lit.Attenuation1 = lit.Attenuation1;
        m_lit.Attenuation2 = lit.Attenuation2;
        m_lit.Theta = lit.Theta;
        m_lit.Phi = lit.Phi;
        return true;
    }
    void destroy(void)
    {
        if (m_pMesh != NULL) {
            m_pMesh->Release();
            m_pMesh = NULL;
        }
    }
    bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return false;

        D3DXVECTOR3 pos(m_bound._center);
        D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
        D3DXVec3TransformCoord(&pos, &pos, &mWorld);
        m_lit.Position = pos;

        pDevice->SetLight(m_index, &m_lit);
        pDevice->LightEnable(m_index, TRUE);
        return true;
    }

    void draw(IDirect3DDevice9* pDevice)
    {
        if (NULL == pDevice)
            return;
        D3DXMATRIX m;
        D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
        pDevice->SetTransform(D3DTS_WORLD, &m);
        pDevice->SetMaterial(&d3d::WHITE_MTRL);
        m_pMesh->DrawSubset(0);
    }

    D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
    DWORD               m_index;
    D3DXMATRIX          m_mLocal;
    D3DLIGHT9           m_lit;
    ID3DXMesh* m_pMesh;
    d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall   g_legoPlane;
CWall   g_legowall[4];

CLight   g_light;

double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
}

// initialization
bool Setup()
{
    int i;

    D3DXMatrixIdentity(&g_mWorld);
    D3DXMatrixIdentity(&g_mView);
    D3DXMatrixIdentity(&g_mProj);

    // create plane and set the position
    if (false == g_legoPlane.create(Device, -1, -1, 9, 0.03f, 6, d3d::GREEN)) return false;
    g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

    // create walls and set the position. note that there are four walls
    if (false == g_legowall[0].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
    g_legowall[0].setPosition(0.0f, 0.12f, 3.06f);
    if (false == g_legowall[1].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
    g_legowall[1].setPosition(0.0f, 0.12f, -3.06f);
    if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
    g_legowall[2].setPosition(4.56f, 0.12f, 0.0f);
    if (false == g_legowall[3].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
    g_legowall[3].setPosition(-4.56f, 0.12f, 0.0f);

    // create four balls and set the position
    for (i = 0; i < 4; i++) {
        if (false == g_sphere[i].create(Device, sphereColor[i])) return false;
        g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
        g_sphere[i].setPower(0, 0);
    }

    // create blue ball for set direction
    if (false == g_target_blueball.create(Device, d3d::BLUE)) return false;
    g_target_blueball.setCenter(.0f, (float)M_RADIUS, .0f);

    // light setting 
    D3DLIGHT9 lit;
    ::ZeroMemory(&lit, sizeof(lit));
    lit.Type = D3DLIGHT_POINT;
    lit.Diffuse = d3d::WHITE;
    lit.Specular = d3d::WHITE * 0.9f;
    lit.Ambient = d3d::WHITE * 0.9f;
    lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
    lit.Range = 100.0f;
    lit.Attenuation0 = 0.0f;
    lit.Attenuation1 = 0.9f;
    lit.Attenuation2 = 0.0f;
    if (false == g_light.create(Device, lit))
        return false;

    // Position and aim the camera.
    D3DXVECTOR3 pos(0.0f, 5.0f, -8.0f);
    D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
    D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
    Device->SetTransform(D3DTS_VIEW, &g_mView);

    // Set the projection matrix.
    D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
        (float)Width / (float)Height, 1.0f, 100.0f);
    Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

    // Set render states.
    Device->SetRenderState(D3DRS_LIGHTING, TRUE);
    Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

    g_light.setLight(Device, g_mWorld);

    score.InitFont(Device);
    player1.InitFont(Device);
    player1.RenderText(380, 10);
    player2.InitFont(Device);
    player2.RenderText(540, 10);
    score.RenderText("score", 480, 10);
    return true;
}

void Cleanup(void)
{
    g_legoPlane.destroy();
    for (int i = 0; i < 4; i++) {
        g_legowall[i].destroy();
    }
    destroyAllLegoBlock();
    g_light.destroy();
    if (g_pFont != NULL) {
        g_pFont->Release();
        g_pFont = NULL;
    }

}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
    int i = 0;
    int j = 0;


    if (Device)
    {
        Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
        Device->BeginScene();

        // update the position of each ball. during update, check whether each ball hit by walls.
        for (i = 0; i < 4; i++) {
            g_sphere[i].ballUpdate(timeDelta);
            for (j = 0; j < 4; j++) { g_legowall[i].hitBy(g_sphere[j]); }
        }

        // check whether any two balls hit together and update the direction of balls
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                if (i >= j) { continue; }
                g_sphere[i].hitBy(g_sphere[j]);
            }
        }
        if (turn == 0 && g_sphere[3].hasIntersected(g_sphere[0])) {
            flag1++;
        }
        else if (turn == 0 && g_sphere[3].hasIntersected(g_sphere[1])) {
            flag2++;
        }
        else if (turn == 0 && g_sphere[3].hasIntersected(g_sphere[2])) {
            flag3++;
        }
        if (turn == 1 && g_sphere[2].hasIntersected(g_sphere[0])) {
            flag1++;
        }
        else if (turn == 1 && g_sphere[2].hasIntersected(g_sphere[1])) {
            flag2++;
        }
        else if (turn == 1 && g_sphere[2].hasIntersected(g_sphere[3])) {
            flag3++;
        }
        if (turn == 0 && g_sphere[0].getVelocity_X() == 0 && g_sphere[0].getVelocity_Z() == 0 && g_sphere[1].getVelocity_X() == 0 && g_sphere[1].getVelocity_Z() == 0 && g_sphere[2].getVelocity_X() == 0 && g_sphere[2].getVelocity_Z() == 0 && g_sphere[3].getVelocity_X() == 0 && g_sphere[3].getVelocity_Z() == 0) {
            if (flag3 == 0) {
                if (flag1 >= 1 && flag2 >= 1) {
                    num1++;
                    flag1 = 0;
                    flag2 = 0;
                    spaceCmp++;
                }
                else if ((flag1 >= 1 && flag2 == 0) || (flag1 == 0 && flag2 >= 1)) {
                    flag1 = 0;
                    flag2 = 0;
                    flag3 = 0;
                    flag3cushion = false; //턴 바뀌면 초기화
                    exitflag = 0;
                    turn = 1 - turn;
                    exitflag = 0;
                    spaceCmp++;
                }
                else if (flag1 == 0 && flag2 == 0 && spaceCmp != spaceCnt) {
                    flag3 = 0;
                    num1--;
                    if (num1 < 0) {
                        num1 = 0;
                    }
                    turn = 1 - turn;
                    exitflag = 0;
                    flag3cushion = false; //턴 바뀌면 초기화
                    spaceCmp = spaceCnt;
                }
            }
            else {
                num1--;
                if (num1 < 0) {
                    num1 = 0;
                }
                flag1 = 0;
                flag2 = 0;
                flag3 = 0;
                turn = 1 - turn;
                exitflag = 0;
                flag3cushion = false; //턴 바뀌면 초기화
                spaceCmp++;
            }
        }
        if (turn == 1 && g_sphere[0].getVelocity_X() == 0 && g_sphere[0].getVelocity_Z() == 0 && g_sphere[1].getVelocity_X() == 0 && g_sphere[1].getVelocity_Z() == 0 && g_sphere[2].getVelocity_X() == 0 && g_sphere[2].getVelocity_Z() == 0 && g_sphere[3].getVelocity_X() == 0 && g_sphere[3].getVelocity_Z() == 0) {
            if (flag3 == 0) {
                if (flag1 >= 1 && flag2 >= 1) {
                    num2++;
                    flag1 = 0;
                    flag2 = 0;
                    spaceCmp++;
                }
                else if ((flag1 >= 1 && flag2 == 0) || (flag1 == 0 && flag2 >= 1)) {
                    flag1 = 0;
                    flag2 = 0;
                    flag3 = 0;
                    flag3cushion = false; //턴 바뀌면 초기화
                    turn = 1 - turn;
                    exitflag = 0;
                    spaceCmp++;
                }
                else if (flag1 == 0 && flag2 == 0 && spaceCmp != spaceCnt) {
                    flag3 = 0;
                    num2--;
                    if (num2 < 0) {
                        num2 = 0;
                    }
                    turn = 1 - turn;
                    exitflag = 0;
                    flag3cushion = false; //턴 바뀌면 초기화
                    spaceCmp = spaceCnt;
                }
            }
            else {
                num2--;
                if (num2 < 0) {
                    num2 = 0;
                }
                flag1 = 0;
                flag2 = 0;
                flag3 = 0;
                turn = 1 - turn;
                exitflag = 0;
                flag3cushion = false; //턴 바뀌면 초기화
                spaceCmp++;
            }
        }
        if (exitflag == 2 && g_sphere[0].getVelocity_X() == 0 && g_sphere[0].getVelocity_Z() == 0 && g_sphere[1].getVelocity_X() == 0 && g_sphere[1].getVelocity_Z() == 0 && g_sphere[2].getVelocity_X() == 0 && g_sphere[2].getVelocity_Z() == 0 && g_sphere[3].getVelocity_X() == 0 && g_sphere[3].getVelocity_Z() == 0) {
            exit(0);
        }
        // draw plane, walls, and spheres
        g_legoPlane.draw(Device, g_mWorld);
        for (i = 0; i < 4; i++) {
            g_legowall[i].draw(Device, g_mWorld);
            g_sphere[i].draw(Device, g_mWorld);
        }
        g_target_blueball.draw(Device, g_mWorld);
        g_light.draw(Device);
        score.RenderText("score", 480, 10);
        player1.RenderText(380, 10);
        player2.RenderText(540, 10);
        Device->EndScene();
        Device->Present(0, 0, 0, 0);
        Device->SetTexture(0, NULL);
    }
    return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool wire = false;
    static bool isReset = true;
    static int old_x = 0;
    static int old_y = 0;
    static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

    switch (msg) {
    case WM_DESTROY:
    {
        ::PostQuitMessage(0);
        break;
    }
    case WM_KEYDOWN:
    {
        switch (wParam) {
        case VK_ESCAPE:
            ::DestroyWindow(hwnd);
            break;
        case VK_RETURN:
            if (NULL != Device) {
                wire = !wire;
                Device->SetRenderState(D3DRS_FILLMODE,
                    (wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
            }
            break;
        case VK_SPACE:
            if (g_sphere[0].getVelocity_X() == 0 && g_sphere[0].getVelocity_Z() == 0 && g_sphere[1].getVelocity_X() == 0 && g_sphere[1].getVelocity_Z() == 0 && g_sphere[2].getVelocity_X() == 0 && g_sphere[2].getVelocity_Z() == 0 && g_sphere[3].getVelocity_X() == 0 && g_sphere[3].getVelocity_Z() == 0) {
                D3DXVECTOR3 targetpos = g_target_blueball.getCenter();
                D3DXVECTOR3   whitepos = g_sphere[3].getCenter();
                D3DXVECTOR3 yellowpos = g_sphere[2].getCenter();
                if (turn == 0) {
                    double theta = acos(sqrt(pow(targetpos.x - whitepos.x, 2)) / sqrt(pow(targetpos.x - whitepos.x, 2) +
                        pow(targetpos.z - whitepos.z, 2)));      // 기본 1 사분면
                    if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x >= 0) { theta = -theta; }   //4 사분면
                    if (targetpos.z - whitepos.z >= 0 && targetpos.x - whitepos.x <= 0) { theta = PI - theta; } //2 사분면
                    if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x <= 0) { theta = PI + theta; } // 3 사분면
                    double distance = sqrt(pow(targetpos.x - whitepos.x, 2) + pow(targetpos.z - whitepos.z, 2));
                    g_sphere[3].setPower(distance * cos(theta), distance * sin(theta));
                }
                else {
                    double theta = acos(sqrt(pow(targetpos.x - yellowpos.x, 2)) / sqrt(pow(targetpos.x - yellowpos.x, 2) +
                        pow(targetpos.z - yellowpos.z, 2)));      // 기본 1 사분면
                    if (targetpos.z - yellowpos.z <= 0 && targetpos.x - yellowpos.x >= 0) { theta = -theta; }   //4 사분면
                    if (targetpos.z - yellowpos.z >= 0 && targetpos.x - yellowpos.x <= 0) { theta = PI - theta; } //2 사분면
                    if (targetpos.z - yellowpos.z <= 0 && targetpos.x - yellowpos.x <= 0) { theta = PI + theta; } // 3 사분면
                    double distance = sqrt(pow(targetpos.x - yellowpos.x, 2) + pow(targetpos.z - yellowpos.z, 2));
                    g_sphere[2].setPower(distance * cos(theta), distance * sin(theta));
                }
                spaceCnt++;
            }
            break;

        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        int new_x = LOWORD(lParam);
        int new_y = HIWORD(lParam);
        float dx;
        float dy;

        if (LOWORD(wParam) & MK_LBUTTON) {

            if (isReset) {
                isReset = false;
            }
            else {
                D3DXVECTOR3 vDist;
                D3DXVECTOR3 vTrans;
                D3DXMATRIX mTrans;
                D3DXMATRIX mX;
                D3DXMATRIX mY;

                switch (move) {
                case WORLD_MOVE:
                    dx = (old_x - new_x) * 0.01f;
                    dy = (old_y - new_y) * 0.01f;
                    D3DXMatrixRotationY(&mX, dx);
                    D3DXMatrixRotationX(&mY, dy);
                    g_mWorld = g_mWorld * mX * mY;

                    break;
                }
            }

            old_x = new_x;
            old_y = new_y;

        }
        else {
            isReset = true;

            if (LOWORD(wParam) & MK_RBUTTON) {
                dx = (old_x - new_x);// * 0.01f;
                dy = (old_y - new_y);// * 0.01f;

                D3DXVECTOR3 coord3d = g_target_blueball.getCenter();
                g_target_blueball.setCenter(coord3d.x + dx * (-0.007f), coord3d.y, coord3d.z + dy * 0.007f);
            }
            old_x = new_x;
            old_y = new_y;

            move = WORLD_MOVE;
        }
        break;
    }
    }

    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
    HINSTANCE prevInstance,
    PSTR cmdLine,
    int showCmd)
{
    srand(static_cast<unsigned int>(time(NULL)));

    if (!d3d::InitD3D(hinstance,
        Width, Height, true, D3DDEVTYPE_HAL, &Device))
    {
        ::MessageBox(0, "InitD3D() - FAILED", 0, 0);
        return 0;
    }

    if (!Setup())
    {
        ::MessageBox(0, "Setup() - FAILED", 0, 0);
        return 0;
    }

    d3d::EnterMsgLoop(Display);

    Cleanup();

    Device->Release();

    return 0;
}