// Offscreen Cairo render of fixed cat poses (no window, no clock) for pixel-identity checks.
//   g++ -std=c++17 -DKUROCAT_SKINSPEC -I linux/src tests/linux-render/render_poses.cpp
//       $(pkg-config --cflags --libs gtk+-3.0 cairo) -o render_poses
//   ./render_poses OUT_DIR                 -> builtin-<coat>-<pose>.png for the 4 built-in coats
//   ./render_poses OUT_DIR skin.kurocat [eyeTheme] -> custom-<pose>.png for one .kurocat (SkinSpec builds only)
// Without -DKUROCAT_SKINSPEC it compiles against the pre-SkinSpec renderer (CatCoatTheme API),
// which is how the "before" frames are produced from an exported HEAD tree.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "CatRenderer.hpp"

static constexpr int kScale = 2;
static constexpr int kW = 360 * kScale, kH = 320 * kScale;

struct Pose { const char* name; int state; int variant; };

static void applyPose(CatPhysics& p, int state, int variant) {
    p.time = 0.7f;
    p.tailPhase = 0.9f;
    switch (state) {
    case 1: p.headYaw.value = -1; p.headPitch.value = -0.65f; p.headTilt.value = -0.13f; break;
    case 2: p.headYaw.value = 1; p.headPitch.value = 0.65f; p.headTilt.value = 0.13f; break;
    case 3:
        p.leftPaw.x.value = 78; p.leftPaw.y.value = 252; p.leftPawOnMouse = true;
        p.rightPaw.x.value = 240; p.rightPaw.y.value = 268; p.rightPawTyping = true;
        p.hindFootActive.value = 1; p.hindFoot.x.value = 164; p.hindFoot.y.value = 270;
        p.bodyTilt.value = 0.12f; p.bodySquash.value = 0.90f; p.tailPhase = 2; break;
    case 4: p.sleepiness = 1; p.earFold.value = 0.25f; break;
    case 5: p.petHeadIntensity = 0.8f; p.earFold.value = 0.6f; break;
    case 6: p.petBodyIntensity = 0.8f; break;
    case 7: p.huntIntensity.value = 1; p.pupilScale.value = 1.12f; p.eyeGaze.x.value = 0.7f; p.eyeGaze.y.value = -0.4f; break;
    case 8: case 9:
        p.headYaw.value = -0.65f; p.headPitch.value = 0.2f; p.headTilt.value = -0.08f;
        if (variant > 0) {
            p.headYaw.value = variant % 2 == 0 ? -1 : 1;
            p.headPitch.value = variant % 4 < 2 ? -1 : 1;
            p.headTilt.value = variant % 2 == 0 ? -0.18f : 0.18f;
            p.bodyTilt.value = variant % 2 == 0 ? -0.15f : 0.15f;
            p.bodySquash.value = variant % 4 < 2 ? 0.85f : 1.08f;
            p.headOffsetY.value = 14;
        }
        if (state == 8) {
            p.leftPaw.x.value = 78; p.leftPaw.y.value = 252; p.leftPawOnMouse = true;
            if (variant > 3) { p.rightPaw.x.value = 240; p.rightPaw.y.value = 262; p.rightPawTyping = true; }
        }
        break;
    case 10: // typing with both paws, ears twitched, mid-wag tail
        p.leftPaw.x.value = 150; p.leftPaw.y.value = 262; p.leftPawTyping = true;
        p.rightPaw.x.value = 226; p.rightPaw.y.value = 258; p.rightPawTyping = true;
        p.earTwitch.value = 0.8f; p.headTilt.value = 0.06f; p.bodySquash.value = 0.95f; p.tailPhase = 3.6f; break;
    default: break;
    }
}

static std::vector<Pose> poses() {
    std::vector<Pose> v = {
        {"idle", 0, 0}, {"turnL", 1, 0}, {"turnR", 2, 0}, {"mouseTypeFoot", 3, 0}, {"sleep", 4, 0},
        {"petHead", 5, 0}, {"petBody", 6, 0}, {"hunt", 7, 0}, {"reach", 8, 0}, {"reachIdle", 9, 0}, {"typing", 10, 0},
    };
    static const char* ext[] = {"", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8"};
    for (int i = 1; i <= 8; ++i) { v.push_back({ext[i], 8, i}); }
    return v;
}

template <class DrawFn>
static void renderTo(const std::string& path, DrawFn draw) {
    cairo_surface_t* s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, kW, kH);
    cairo_t* cr = cairo_create(s);
    cairo_scale(cr, kScale, kScale);
    draw(cr);
    cairo_destroy(cr);
    cairo_surface_flush(s);
    cairo_surface_write_to_png(s, path.c_str());
    cairo_surface_destroy(s);
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: %s OUT_DIR [skin.kurocat]\n", argv[0]); return 2; }
    std::string out = argv[1];
    static const char* coatNames[] = {"black", "siamese", "shorthair", "orangeWhite"};
    static const EyeColorTheme coatEyes[] = {EyeColorTheme::NeonChartreuse, EyeColorTheme::SiameseBlue,
                                             EyeColorTheme::SoftAmber, EyeColorTheme::ReferenceHazel};
    int frames = 0;
#ifdef KUROCAT_SKINSPEC
    if (argc >= 3) {
        std::string error;
        SkinSpec skin;
        if (!SkinSpec::loadFile(argv[2], skin, error)) { std::fprintf(stderr, "skin rejected: %s\n", error.c_str()); return 1; }
        CatSettings settings; settings.showKeyLegends = false;
        settings.eyeTheme = argc >= 4 ? (EyeColorTheme)std::atoi(argv[3]) : EyeColorTheme::SoftAmber;
        for (const Pose& pose : poses()) {
            CatPhysics p; applyPose(p, pose.state, pose.variant);
            std::string name = std::string(pose.name[0] ? pose.name : "x");
            renderTo(out + "/custom-" + name + ".png", [&](cairo_t* cr) { CatRenderer::draw(cr, p, settings, skin); });
            ++frames;
        }
        std::printf("rendered %d frames of %s\n", frames, skin.name.c_str());
        return 0;
    }
#endif
    for (int coat = 0; coat < 4; ++coat) {
        for (const Pose& pose : poses()) {
            CatPhysics p; applyPose(p, pose.state, pose.variant);
            CatSettings settings; settings.showKeyLegends = false;
            settings.coatTheme = (CatCoatTheme)coat; settings.eyeTheme = coatEyes[coat];
            std::string file = out + "/builtin-" + coatNames[coat] + "-" + pose.name + ".png";
#ifdef KUROCAT_SKINSPEC
            const SkinSpec& skin = builtInSkin((CatCoatTheme)coat);
            renderTo(file, [&](cairo_t* cr) { CatRenderer::draw(cr, p, settings, skin); });
#else
            renderTo(file, [&](cairo_t* cr) { CatRenderer::draw(cr, p, settings); });
#endif
            ++frames;
        }
        // Petting hit zones on a 2pt grid for a few poses ('.' none, 'h' head, 'b' body).
        FILE* hit = std::fopen((out + "/builtin-" + coatNames[coat] + "-hit.txt").c_str(), "w");
        for (int state : {0, 1, 2, 3}) {
            CatPhysics p; applyPose(p, state, 0);
            for (int y = 0; y < 320; y += 2) {
                for (int x = 0; x < 360; x += 2) {
#ifdef KUROCAT_SKINSPEC
                    CatHitZone z = CatRenderer::hitTest({(float)x, (float)y}, p, builtInSkin((CatCoatTheme)coat));
#else
                    CatHitZone z = CatRenderer::hitTest({(float)x, (float)y}, p, (CatCoatTheme)coat);
#endif
                    std::fputc(z == CatHitZone::Head ? 'h' : (z == CatHitZone::Body ? 'b' : '.'), hit);
                }
                std::fputc('\n', hit);
            }
        }
        std::fclose(hit);
    }
    std::printf("rendered %d built-in frames\n", frames);
    return 0;
}
