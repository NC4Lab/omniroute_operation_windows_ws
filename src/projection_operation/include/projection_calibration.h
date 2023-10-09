// ##########################################################################################################

// ======================================== projection_calibration.h ========================================

// ##########################################################################################################

#ifndef _PROJECTION_CALIBRATION_H
#define _PROJECTION_CALIBRATION_H

// ================================================== INCLUDE ==================================================

// Local custom libraries
#include "projection_utils.h"

// ================================================== VARIABLES ==================================================

// Specify the window name
std::string windowName = "Projection Calibration";

// Variables related to control point parameters
/// @todo: clearify the units of these variables
const float cp_size = 0.015f;
const float cp_xy_lim = 0.5f;
const float cp_height = cp_size * PROJ_WIN_ASPECT_RATIO * 1.8 * 0.75;

// Control point parameter arrays
/**
 * @brief Array to hold the position and transformation parameters for control points in normalized coordinates [-1, 1].
 *
 * Each row corresponds to a specific control point on the screen, and each column holds a different attribute
 * of that control point.
 *
 * - Rows:
 *   - [0]: Top-left control point
 *   - [1]: Top-right control point
 *   - [2]: Bottom-right control point
 *   - [3]: Bottom-left control point
 *
 * - Columns:
 *   - [0]: X-distance from center [-1,1]
 *   - [1]: Y-distance from center [-1,1]
 *   - [2]: Width/Radius parameter
 *   - [3]: Height parameter
 *   - [4]: Shearing factor
 */
float cpParam_default[4][5] = { // Default control point parameters
    {-cp_xy_lim, cp_xy_lim, cp_size, cp_height, 0.0f}, // top-left control point
    {cp_xy_lim, cp_xy_lim, cp_size, cp_height, 0.0f},  // top-right control point
    {cp_xy_lim, -cp_xy_lim, cp_size, cp_height, 0.0f}, // bottom-right control point
    {-cp_xy_lim, -cp_xy_lim, cp_size, cp_height, 0.0f} // bottom-left control point
};
float cpParam[4][5]; // Dynamic array to hold the control point parameters

// Other variables related to control points
int cpSelected = 0;
std::string cpModMode = "position";
std::vector<float> cpActiveRGB = {1.0f, 0.0f, 0.0f};   // Active control point marker color
std::vector<float> cpInactiveRGB = {0.0f, 0.0f, 1.0f}; // Inactive control point marker color

// The homography matrix used to warp perspective.
cv::Mat H = cv::Mat::eye(3, 3, CV_32F);

// Directory paths
std::string package_path = ros::package::getPath("projection_operation");
std::string workspacePath = package_path.substr(0, package_path.rfind("/src"));
std::string configDirPath = workspacePath + "/data/proj_cfg";
std::string imgTestPath = workspacePath + "/data/img/test_patterns";
std::string imgStatePath = workspacePath + "/data/img/ui_state_images";

// Test image variables
std::vector<ILuint> imgTestIDs; // Container to hold the loaded images
std::vector<std::string> imgTestPaths = {
    // List of test image file paths
    imgTestPath + "/1_test_pattern.bmp",
    imgTestPath + "/2_manu_pirate.bmp",
    imgTestPath + "/3_earthlings.bmp",
};
int imgTestInd = 0;                    // Index of the image to be loaded
size_t nTestImg = imgTestPaths.size(); // Number of test images

// Monitor variables
std::vector<ILuint> imgMonIDs; // Container to hold the loaded images for ui
std::vector<std::string> imgMonPaths = {
    // List of monitor number image file paths
    imgStatePath + "/m0.bmp",
    imgStatePath + "/m1.bmp",
    imgStatePath + "/m2.bmp",
    imgStatePath + "/m3.bmp",
    imgStatePath + "/m4.bmp",
    imgStatePath + "/m5.bmp",
};
int imgMonInd = 0;         // Index of the image to be loaded
int nMonitors;             // Number of monitors connected to the system
bool isFullScreen = false; // Flag to indicate if the window is in full screen mode

// Control point parameter image variables for ui
std::vector<ILuint> imgParamIDs; // Container to hold the loaded images for ui
std::vector<std::string> imgParamPaths = {
    // List of cp parameter image file paths
    imgStatePath + "/p.bmp",
    imgStatePath + "/d.bmp",
    imgStatePath + "/s.bmp",
};
int imgParamInd = 0; // Index of the image to be loaded

// Callibration image variables for ui
std::vector<ILuint> imgCalIDs; // Container to hold the loaded images for ui
std::vector<std::string> imgCalPaths = {
    // List of mode image file paths
    imgStatePath + "/c-wm.bmp",
    imgStatePath + "/c-wl.bmp",
    imgStatePath + "/c-wr.bmp",
    imgStatePath + "/c-f.bmp",
    imgStatePath + "/c-d.bmp",
};
int imgCalInd = 0;                     // Index of the image to be loaded
size_t nCalModes = imgCalPaths.size(); // Number of calibration modes

// Variables related to window and OpenGL
GLFWwindow *window;
GLuint fboTexture;
GLFWmonitor *monitor = NULL;
GLFWmonitor **monitors;

// ================================================== FUNCTIONS ==================================================

/**
 * @brief GLFW key callback function to handle key events and execute corresponding actions.
 *
 * This function is set as the GLFW key callback and gets called whenever a key event occurs.
 * It handles various key events for control points, monitor handling, XML operations, and more.
 *
 * ## Keybindings:
 * - **Any Key Release Action:**
 *   - `R`: Reset control point parameters.
 *   - `F1-F4`: Select a control point (Top-Left, Top-Right, Bottom-Right, Bottom-Left).
 *   - `A, D, S`: Change calibration point parameters.
 *   - `F`: Toggle fullscreen mode.
 *   - `M`: Move window to another monitor.
 *   - `0-5`: Select monitor index.
 *   - `Enter`: Save coordinates to XML.
 *   - `L`: Load coordinates from XML.
 *
 * - **Any Key Press or Repeat Action:**
 *   - `Alt + Left/Right`: Change calibration mode.
 *   - `Ctrl + Left/Right`: Change displayed image.
 *   - `Shift or no modifier + Arrow keys`: Adjust control point position, dimension, or shear.
 *
 * @param window Pointer to the GLFW window that received the event.
 * @param key The keyboard key that was pressed or released.
 * @param scancode The system-specific scancode of the key.
 * @param action GLFW_PRESS, GLFW_RELEASE or GLFW_REPEAT.
 * @param mods Bit field describing which modifier keys were held down.
 */
void callbackKeyBinding(GLFWwindow *, int, int, int, int);

/**
 * @brief Callback function for handling framebuffer size changes.
 *
 * This function is called whenever the framebuffer size changes,
 * and it updates the OpenGL viewport to match the new dimensions.
 *
 * @param window Pointer to the GLFW window.
 * @param width The new width of the framebuffer.
 * @param height The new height of the framebuffer.
 */
void callbackFrameBufferSizeGLFW(GLFWwindow *, int, int);

/**
 * @brief Callback function for handling errors.
 *
 * This function is called whenever an error occurs in the GLFW context.
 * It logs the error message using ROS_ERROR.
 *
 * @param error The error code.
 * @param description The error description.
 */
static void callbackErrorGLFW(int, const char *);

/**
 * @brief Draws a control point as a quadrilateral using OpenGL.
 *
 * This function uses OpenGL to draw a quadrilateral that represents a control point.
 * The control point is drawn in a clockwise direction, starting from the bottom-left corner.
 *
 * @param x The x-coordinate of the bottom-left corner of the control point.
 * @param y The y-coordinate of the bottom-left corner of the control point.
 * @param radius The radius of the control point.
 * @param rgb Vector of rgb values to color the marker.
 */
void drawControlPoint(float, float, float, std::vector<float>);

/**
 * @brief Draws a textured wall using OpenGL.
 *
 * @param img_vertices Vector of vertex/corner points for the wall.
 */
void drawRectImage(std::vector<cv::Point2f>);

/**
 * @brief Draws all walls in the maze grid with texture mapping and perspective warping.
 *
 * This function iterates through the maze grid to draw each wall. It uses the DevIL library
 * to handle image loading and OpenGL for rendering. The function also performs perspective
 * warping based on the homography matrix and shear and height values extracted from control points.
 * 
 * @param ref_H The homography matrix used to warp perspective.
 * @param cp_param The array of control point parameters.
 * @param fbo_texture The OpenGL texture ID of the framebuffer object.
 * @param img_base_id The DevIL image ID of the base image.
 * @param img_mon_id The DevIL image ID of the monitor image.
 * @param img_param_id The DevIL image ID of the parameter image.
 * @param img_cal_id The DevIL image ID of the calibration image.
 */
void drawWallsAll(cv::Mat &, float[4][5], GLuint, ILuint, ILuint, ILuint, ILuint);

/**
 * @brief Changes the display mode and monitor of the application window.
 *
 * This function switches the application window between full-screen and windowed modes
 * and moves it to the monitor specified by the global variable imgMonNumInd.
 *
 * In full-screen mode, the window is resized to match the dimensions of the selected monitor.
 * In windowed mode, the window is resized to a default size and positioned near the top-left
 * corner of the selected monitor.
 *
 * @note The global variables monitor, monitors, imgMonNumInd, window, and isFullScreen are
 *       used to control the behavior of this function.
 *       Will only exicute if monotor parameters have changed.
 * 
 * @param ref_monitor Reference to the GLFWmonitor pointer that will be updated.
 * @param ref_monitors Reference to the GLFWmonitor pointer array.
 * @param is_fullscreen Boolean flag indicating whether the window should be set to full-screen mode.
 * @param imp_mon_ind Index of the monitor to move the window to.
 */
void updateWindowMonMode(GLFWmonitor *&, GLFWmonitor **&, bool, int);

/**
 * @brief Computes the homography matrix based on control points and wall image vertices.
 *
 * This function calculates the homography matrix that maps points from the source image (wall images)
 * to the destination image (control points). The homography matrix is stored in the global variable H.
 *
 * Control points are specified in normalized coordinates and are fetched from the global variable cpParam.
 * Wall image vertices are calculated based on the dimensions and spacing of the maze walls.
 *
 * @note This function uses the OpenCV library to compute the homography matrix.
 * @note The global variables cpParam, MAZE_SIZE, and wallSpace are used to control the behavior of this function.
 */
void computeHomography();

/**
 * @brief Used to reset control point parameter list.
 */
void resetParamCP();

/**
 * @brief  Entry point for the projection_calibration ROS node.
 *
 * This program initializes ROS, DevIL, and GLFW, and then enters a main loop
 * to handle image projection and calibration tasks.
 *
 * @param  argc  Number of command-line arguments.
 * @param  argv  Array of command-line arguments.
 *
 * @return 0 on successful execution, -1 on failure.
 */
int main(int, char **);

#endif