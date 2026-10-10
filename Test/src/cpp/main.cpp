#include <fstream>
#include "parser.h"
#include "AST.h"
#include "Compiler.h"
#include "CPUSL.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <chrono>

#define MINIAUDIO_IMPLEMENTATION
#include "../h/miniaudio.h"


void printExpr(const std::shared_ptr<Expr> expr, const std::string& prefix = "", bool isLast = true)
{
    if (!expr)
        return;

    std::cout << prefix
              << (isLast ? "└── " : "├── ");

    const std::string childPrefix =
        prefix + (isLast ? "    " : "│   ");

    switch (expr->type)
    {
        case LITERAL:
        {
            const auto e = std::static_pointer_cast<LiteralExpr>(expr);

            std::cout << "Literal: "
                      << e->tok.lexeme
                      << '\n';

            break;
        }

        case DECLARATION:
        {
            const auto e = std::static_pointer_cast<IdentifierExpr>(expr);

            std::cout << "Declaration: "
                      << e->tok.lexeme
                      << '\n';

            break;
        }

        case IDENTIFIER:
        {
            const auto e = std::static_pointer_cast<IdentifierExpr>(expr);

            std::cout << "Identifier: "
                      << e->tok.lexeme
                      << '\n';

            break;
        }

        case ACCESSOR:
        {
            const auto e = std::static_pointer_cast<AccessorExpr>(expr);

            std::cout << "Accessor: "
                      << e->name
                      << "."
                      << e->tok.lexeme
                      << '\n';

            break;
        }

        case UNARY:
        {
            const auto e = std::static_pointer_cast<UnaryExpr>(expr);

            std::cout << "Unary: "
                      << e->tok.lexeme
                      << '\n';

            printExpr(e->expr, childPrefix, true);

            break;
        }

        case BINARY:
        {
            const auto e = std::static_pointer_cast<BinaryExpr>(expr);

            std::cout << "Binary: "
                      << e->tok.lexeme
                      << '\n';

            printExpr(e->lhs, childPrefix, false);
            printExpr(e->rhs, childPrefix, true);

            break;
        }

        case FUNCTION:
        {
            const auto e = std::static_pointer_cast<FunctionCallExpr>(expr);

            std::cout << "Function: "
                      << e->tok.lexeme
                      << '\n';

            for (std::size_t i = 0; i < e->args.size(); ++i)
            {
                printExpr(
                    e->args[i],
                    childPrefix,
                    i == e->args.size() - 1
                );
            }

            break;
        }
    }
}

void TestSourceCode(const char* _source)
{
    Lexer lexer;
    lexer.set_source(_source);
    lexer.scan_source();
    Parser parser(lexer.get_tokens());
    
    parser.parse();
    auto statements = parser.exprs["const"].statements;
    int size = statements.size();
    std::cout << "CONST:" << std::endl;
    for (int i = 0; i < size; i++)
    {
        printExpr(statements[i]);
    }
    auto statements2 = parser.exprs["entry"].statements;
    size = statements2.size();
    std::cout << "ENTRY:" << std::endl;
    for (int i = 0; i < size; i++)
    {
        printExpr(statements2[i]);
    }
    auto statements3 = parser.exprs["ps"].statements;
    size = statements3.size();
    std::cout << "PS:" << std::endl;
    for (int i = 0; i < size; i++)
    {
        printExpr(statements3[i]);
    }

    AST ast;
    ast.exprs = parser.exprs;
    ast.check_semantics();

    for (std::vector<VariableInfo>::iterator it = ast.const_variables.begin(); it != ast.const_variables.end(); it++)
    {
        std::cout << it->name << "(" << it->precomputed_value.x << ", " << it->precomputed_value.y << ", " << it->precomputed_value.z << ", " << it->precomputed_value.w << ")" << std::endl;
    }

    if (ast.has_errors()) return;

    Compiler compiler;
    compiler.ast = &ast;
    compiler.compile("");
    
}

void REPL()
{
    std::string total_input;
    while (true)
    {
        std::string current_input;
        //input = "";
        std::getline(std::cin, current_input);

        if (current_input.size() == 4 && memcmp(current_input.c_str(), "/end", 4) == 0) break;
        if (current_input.size() == 12 && memcmp(current_input.c_str(), "/save_source", 12) == 0)
        {
            // Save source
            std::ofstream sourceFile;
            sourceFile.open("SavedSource.txt");
            sourceFile.write(total_input.c_str(), total_input.size());
            sourceFile.close();
            break;
        }

        total_input += current_input;
        TestSourceCode(total_input.c_str());
    }
}

static bool read_ppm_int(std::istream& in, int& value)
{
    while (in >> std::ws && in.peek() == '#')
        in.ignore(100000, '\n');
    return static_cast<bool>(in >> value);
}

static bool load_ppm(const std::string& path, std::vector<Color>& pixels,
    uint32_t& width, uint32_t& height)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;

    std::string magic;
    in >> magic;
    if (magic != "P6")
        return false;

    int w = 0, h = 0, maxval = 0;
    if (!read_ppm_int(in, w) || !read_ppm_int(in, h) || !read_ppm_int(in, maxval))
        return false;
    if (w <= 0 || h <= 0 || maxval <= 0 || maxval > 255)
        return false;
    in.get(); // un seul espace blanc après maxval

    std::vector<unsigned char> raw(static_cast<size_t>(w) * h * 3);
    in.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(raw.size()));
    if (static_cast<size_t>(in.gcount()) != raw.size())
        return false;

    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
    pixels.resize(static_cast<size_t>(w) * h);
    for (size_t i = 0; i < pixels.size(); ++i) {
        pixels[i].f[0] = raw[i * 3 + 0] / static_cast<float>(maxval);
        pixels[i].f[1] = raw[i * 3 + 1] / static_cast<float>(maxval);
        pixels[i].f[2] = raw[i * 3 + 2] / static_cast<float>(maxval);
        pixels[i].f[3] = 1.0f;
    }
    return true;
}

class Demo
{
public:
    const int width = 240;
    const int height = 240;
    CPUSL* cpusl_instance;
    std::vector<Color> pixels;
    std::vector<uint32_t> display_pixels;

    ma_result result;
    ma_encoder_config encoderConfig;
    ma_encoder encoder;
    ma_device_config deviceConfig;
    ma_device device;
    float rms;
    Demo();
};



Demo demo;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    if (pInput == nullptr) {
        return;
    }

    const float* pSamples = (const float*)pInput;
    ma_uint32 totalSamples = frameCount * pDevice->capture.channels;

    float peak = 0.0f;
    float sumSquares = 0.0f;

    for (ma_uint32 i = 0; i < totalSamples; ++i) {
        float sample = pSamples[i];

        // Track Peak
        if (sample < 0) sample = -sample;
        if (sample > peak) {
            peak = sample;
        }

        // Accumulate for RMS
        sumSquares += sample * sample;
    }

    demo.rms = sqrtf(sumSquares / totalSamples);

    (void)pOutput;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Describe our pixel buffer to GDI.
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = demo.width;
        bmi.bmiHeader.biHeight = -demo.height;   // negative = top-down rows
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        RECT rc;
        GetClientRect(hwnd, &rc);

        // COLORONCOLOR keeps pixels crisp when scaling up (no smoothing).
        SetStretchBltMode(hdc, COLORONCOLOR);
        StretchDIBits(hdc,
            0, 0, rc.right, rc.bottom,   // destination rect
            0, 0, demo.width, demo.height,          // source rect
            demo.display_pixels.data(), &bmi,
            DIB_RGB_COLORS, SRCCOPY);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "Window";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);

    // Size the window so the *client area* matches the scaled image.
    RECT rc = { 0, 0, demo.width * 2, demo.height * 2 };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rc, style, FALSE);

    HWND hwnd = CreateWindowExA(0, "Window", "test_cpusl", style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);
    if (!hwnd)
        return 1;

    ShowWindow(hwnd, nCmdShow);

    std::chrono::high_resolution_clock clock;
    std::chrono::high_resolution_clock::time_point start = clock.now();

    uint32_t image_width = 1365;
    uint32_t image_height = 2048;
    Texture imageTexture;
    imageTexture.width = image_width;
    imageTexture.height = image_height;
    imageTexture.pixels = std::vector<Color>(image_width * image_height);
    bool res = load_ppm(" " /* Your image here */, imageTexture.pixels, image_width, image_height);
    //if (res == false) return;

    MSG msg;
    bool running = true;
    while (running)
    {
        while(PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        float time = (std::chrono::duration_cast<std::chrono::milliseconds>(clock.now() - start).count() / 1000.0);

        float entry = demo.rms * 2;

        CPUSL_execute(demo.cpusl_instance, "win_source.cpusl", demo.pixels.data(), demo.width, demo.height, &entry, sizeof(float), &imageTexture, 1);

        for (int i = 0; i < demo.width * demo.height; i++)
        {
            demo.display_pixels[i] = ((uint8_t)(demo.cpusl_instance->output[i].r * 255 + 0.5f)) << 16 |
                ((uint8_t)(demo.cpusl_instance->output[i].g * 255 + 0.5f)) << 8 |
                ((uint8_t)(demo.cpusl_instance->output[i].b * 255 + 0.5f)) << 0 |
                ((uint8_t)(demo.cpusl_instance->output[i].a * 255 + 0.5f)) << 24;
        }

        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
    }
    return static_cast<int>(msg.wParam);
}

Demo::Demo()
{
    display_pixels = std::vector<uint32_t>(width * height);
    pixels = std::vector<Color>(width * height);
    std::fill(pixels.begin(), pixels.begin() + width * height, Color());
    cpusl_instance = CPUSL_create();
    CPUSL_compile(cpusl_instance, "win_source_code.txt", "./", "win_source");

    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format = encoder.config.format;
    deviceConfig.capture.channels = encoder.config.channels;
    deviceConfig.sampleRate = encoder.config.sampleRate;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &encoder;


    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize capture device.\n");
    }

    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        ma_device_uninit(&device);
        printf("Failed to start device.\n");
    }
}