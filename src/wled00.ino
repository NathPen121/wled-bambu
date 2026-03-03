/*
 * WLED Bambu — Bambu Lab Printer Status LED Controller
 *
 * This file is NOT compiled directly. The patch_wled.py script injects
 * the Bambu calls into WLED's wled_main.cpp at build time:
 *
 *   setup(): setupBambuWebRoutes(); loadDefaultBambuEffects();
 *   loop():  pollBambu(); applyBambuEffects();
 *
 * See tools/patch_wled.py for details.
 */
