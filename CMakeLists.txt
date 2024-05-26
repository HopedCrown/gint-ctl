# gintctl: gint's control application

cmake_minimum_required(VERSION 3.15)
project(gintctl VERSION 2.10.0 LANGUAGES C ASM)

include(GenerateG1A)
include(GenerateG3A)
include(Fxconv)

find_package(Gint 2.8 REQUIRED)
find_package(LibProf 2.1 REQUIRED)
find_package(JustUI 1.0 REQUIRED)

set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)

set(SOURCES
  src/gintctl.c
  src/menu.c
  src/plot.c
  src/usb_commands.c
  src/util.c
  src/gint/bfile.c
  src/gint/cpumem.c
  src/gint/dma.c
  src/gint/drivers.c
  src/gint/dsp.c
  src/gint/dsp.s
  src/gint/dump.c
  src/gint/gdb.S
  src/gint/gdb.c
  src/gint/gray.c
  src/gint/image.c
  src/gint/keyboard.c
  src/gint/kmalloc.c
  src/gint/overclock.c
  src/gint/ram.c
  src/gint/render.c
  src/gint/rtc.c
  src/gint/spuram.c
  src/gint/syscallemu.c
  src/gint/timer.c
  src/gint/timer_callbacks.c
  src/gint/tlb.c
  src/gint/topti.c
  src/gint/usb.c
  src/gint/usbtrace.c
  src/mem/mem.c
  src/perf/cpucache.c
  src/perf/cpucache.S
  src/perf/cpu.c
  src/perf/cpu.S
  src/perf/interrupt.c
  src/perf/libprof.c
  src/perf/memory.c
  src/perf/memory.S
  src/perf/render.c
  src/regs/regs.c
  src/widgets/gscreen.c
  src/widgets/gtable.c
)
set(ASSETS_fx
  assets-fx/fonts/hexa.png
  assets-fx/fonts/mini.png
  assets-fx/fonts/title.png
  assets-fx/fonts/uf5x7
  assets-fx/img/bopti_1col.png
  assets-fx/img/bopti_2col.png
  assets-fx/img/bopti_3col.png
  assets-fx/img/kbd_events.png
  assets-fx/img/kbd_pressed.png
  assets-fx/img/kbd_released.png
  assets-fx/img/libimg_swords.png
  assets-fx/img/opt_dump.png
  assets-fx/img/opt_gint_bfile.png
  assets-fx/img/opt_gint_bopti.png
  assets-fx/img/opt_gint_cpumem.png
  assets-fx/img/opt_gint_dma.png
  assets-fx/img/opt_gint_drivers.png
  assets-fx/img/opt_gint_gdb.png
  assets-fx/img/opt_gint_gray.png
  assets-fx/img/opt_gint_keyboard.png
  assets-fx/img/opt_gint_kmalloc.png
  assets-fx/img/opt_gint_ram.png
  assets-fx/img/opt_gint_rtc.png
  assets-fx/img/opt_gint_spuram.png
  assets-fx/img/opt_gint_timer_callbacks.png
  assets-fx/img/opt_gint_timers.png
  assets-fx/img/opt_gint_tlb.png
  assets-fx/img/opt_gint_usb.png
  assets-fx/img/opt_main.png
  assets-fx/img/opt_mem.png
  assets-fx/img/opt_perf_cpu.png
  assets-fx/img/opt_perf_libprof.png
  assets-fx/img/opt_perf_memory.png
  assets-fx/img/opt_perf_memory_sh3.png
  assets-fx/img/opt_perf_render.png
  assets-fx/img/profile_gray_alpha.png
  assets-fx/img/profile_gray.png
  assets-fx/img/profile_mono_alpha.png
  assets-fx/img/profile_mono.png
  assets-fx/img/rtc_arrows.png
  assets-fx/img/rtc_segments.png
  assets-fx/img/tlb_cells.png
)
set(ASSETS_RGB
  assets-cg/fonts/font8x9_bold.png
  assets-cg/img/kbd_events.png
  assets-cg/img/libimg_even_odd.png
  assets-cg/img/libimg_odd_even.png
  assets-cg/img/libimg_sq_even.png
  assets-cg/img/libimg_sq_odd.png
  assets-cg/img/libimg_train.png
  assets-cg/img/rtc_arrows.png
  assets-cg/img/swords.png
)
set(ASSETS_cg
  ${ASSETS_RGB}
  assets-cg/img/kbd_sprite_cg.png)

fxconv_declare_converters(assets-cg/converters.py)
fxconv_declare_assets(${ASSETS_fx} ${ASSETS_cg} WITH_METADATA)

add_executable(gintctl ${SOURCES} ${ASSETS_${FXSDK_PLATFORM}})
target_compile_options(gintctl PRIVATE
  -Wall -Wextra -Os -g -Wno-error=implicit-function-declaration)
target_link_options(gintctl PRIVATE
  -Wl,-Map=map -Wl,--print-memory-usage)
target_include_directories(gintctl PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gintctl
  LibProf::LibProf JustUI::JustUI Gint::Gint)

if("${FXSDK_PLATFORM_LONG}" STREQUAL fx9860G)
  generate_g1a(TARGET gintctl OUTPUT "gintctl.g1a"
    NAME "gintctl" INTERNAL "@GINTCTL" ICON assets-fx/icon.png)
elseif("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  generate_g3a(TARGET gintctl OUTPUT "gintctl.g3a"
    NAME "" ICONS assets-cg/icon-uns.png assets-cg/icon-sel.png)
  add_custom_command(TARGET gintctl POST_BUILD
    COMMAND echo -n "g3a file size: "
    COMMAND stat -c %s "${CMAKE_CURRENT_SOURCE_DIR}/gintctl.g3a")
elseif("${FXSDK_PLATFORM_LONG}" STREQUAL fx9860G_G3A)
  generate_g3a(TARGET gintctl OUTPUT "gintctl-fx.g3a"
    NAME "" ICONS assets-cg/icon-uns.png assets-cg/icon-sel.png)
endif()
