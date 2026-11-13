#include <assert.h>
#include "scale_math.h"

int main(void)
{
    assert(fhd_dialog_width(428, 607, 100) == 428);
    assert(fhd_dialog_width(428, 607, 115) == 492);
    assert(fhd_dialog_width(428, 607, 130) == 556);
    assert(fhd_dialog_width(428, 607, 150) == 642);
    assert(fhd_dialog_width(428, 80, 150) == 508);
    assert(fhd_dialog_width(428, 0, 150) == 428);
    assert(fhd_dialog_width(428, 607, 151) == 428);
    assert(fhd_wide_x(0, 428, 642) == 0);
    assert(fhd_wide_x(428, 428, 642) == 642);
    assert(fhd_wide_x(338, 428, 642) == 507);
    assert(fhd_wide_x(-17, 428, 642) == -26);
    assert(fhd_gov_width(411, 507, 1920, 150) == 616);
    assert(fhd_gov_width(411, 59, 1024, 150) == 504);
    assert(fhd_hud_percent(3840, 200) == 200);
    assert(fhd_hud_percent(1024, 200) == 100);
    assert(fhd_nav_x(100, 3840, 200) == 200);
    assert(fhd_nav_x(3715, 3840, 200) == 3590);
    assert(fhd_nav_source_x(3590, 3840, 200) == 3715);
    Rect a = {-1920, 0, 0, 1080};
    assert(fhd_equal_rect(a, a));
    assert(!fhd_equal_rect(a, (Rect){0, 0, 1920, 1080}));
    return 0;
}
