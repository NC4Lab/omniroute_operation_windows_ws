// ########################################################################################################

// ======================================== projection_display.cpp ========================================

// ########################################################################################################

// ================================================== INCLUDE ==================================================

#include "projection_display.h"

// ================================================== FUNCTIONS ==================================================

void callbackKeyBinding(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // Set the current OpenGL context to the window
    glfwMakeContextCurrent(window);

    // _______________ ANY KEY RELEASE ACTION _______________

    if (action == GLFW_RELEASE)
    {

        // ----------Set/unset Fullscreen [F] ----------

        if (key == GLFW_KEY_F)
        {
            F.fullscreen_mode = !F.fullscreen_mode;
            F.change_window_mode = true;
        }

        // ----------Check for move monitor command [M] ----------

        if (key == GLFW_KEY_M)
        {
            F.windows_set_to_proj = !F.windows_set_to_proj;
            F.change_window_mode = true;
        }
    }
}

int updateTexture(
    int proj_mon_ind,
    const std::vector<cv::Mat> &_wallImgMatVec,
    const std::vector<cv::Mat> &_floorImgMatVec,
    const std::vector<std::array<std::array<std::array<cv::Mat, MAZE_SIZE>, MAZE_SIZE>, N_CAL_MODES>> &_HMAT_ARR_VEC,
    MazeRenderContext &out_projCtx)
{
    // Initialize the image to be used as the texture
    cv::Mat img_merge = cv::Mat::zeros(out_projCtx.windowHeight, out_projCtx.windowWidth, CV_8UC4);

    // Iterate through through calibration modes in descending order so floor is drawn first
    for (int cal_i = N_CAL_MODES - 1; cal_i >= 0; --cal_i)
    {
        CalibrationMode _CAL_MODE = static_cast<CalibrationMode>(cal_i);

        // Specify number of rows/cols to loop through
        int grid_size = (_CAL_MODE == WALLS_LEFT || _CAL_MODE == WALLS_MIDDLE || _CAL_MODE == WALLS_RIGHT) ? MAZE_SIZE : 1;

        // Iterate through the maze grid rows
        for (int gr_i = 0; gr_i < grid_size; gr_i++) // image bottom to top
        {
            // Iterate through each column in the maze row
            for (int gc_i = 0; gc_i < grid_size; gc_i++) // image left to right
            {
                // Get the wall image to be used
                cv::Mat img_copy;
                if (_CAL_MODE == WALLS_LEFT || _CAL_MODE == WALLS_MIDDLE || _CAL_MODE == WALLS_RIGHT)
                {
                    int img_ind = WALL_IMG_PROJ_MAP[proj_mon_ind][gr_i][gc_i][_CAL_MODE];
                    if (_wallImgMatVec[img_ind].empty())
                    {
                        ROS_ERROR("[updateTexture] Stored OpenCV wall image is empty: Window[%d] Monitor[%d] Wall[%d][%d] Calibration[%d] Image[%d]",
                                  out_projCtx.windowInd, proj_mon_ind, gr_i, gc_i, _CAL_MODE, img_ind);
                        return -1;
                    }
                    else
                    {
                        _wallImgMatVec[img_ind].copyTo(img_copy);
                    }
                }
                else
                {
                    int img_ind = FLOOR_IMG_PROJ_MAP[proj_mon_ind];
                    if (_floorImgMatVec[img_ind].empty())
                    {
                        ROS_ERROR("[updateTexture] Stored OpenCV floor image is empty: Window[%d] Monitor[%d] Wall[%d][%d] Calibration[%d] Image[%d]",
                                  out_projCtx.windowInd, proj_mon_ind, gr_i, gc_i, _CAL_MODE, img_ind);
                        return -1;
                    }
                    else
                    {
                        _floorImgMatVec[img_ind].copyTo(img_copy);
                    }
                }

                // Get homography matrix for this wall
                cv::Mat H = _HMAT_ARR_VEC[proj_mon_ind][_CAL_MODE][gr_i][gc_i];

                // Warp Perspective
                cv::Mat img_warp;
                if (warpImgMat(img_copy, H, img_warp) < 0)
                {
                    ROS_ERROR("[updateTexture] Warp image error: Window[%d] Monitor[%d] Wall[%d][%d] Calibration[%d]",
                              out_projCtx.windowInd, proj_mon_ind, gr_i, gc_i, _CAL_MODE);
                    return -1;
                }

                // Merge the warped image with the final image
                if (mergeImgMat(img_warp, img_merge) < 0)
                    return -1;
            }
        }
    }

    // Load the new texture and return status
    return out_projCtx.loadMatTexture(img_merge);
}

void appLoadAssets()
{

    // Load images using OpenCV
    if (loadImgMat(fiImgPathWallVec, wallImgMatVec) < 0)
        throw std::runtime_error("[appLoadAssets] Failed to load OpentCV wall images");
    if (loadImgMat(fiImgPathFloorVec, floorImgMatVec) < 0)
        throw std::runtime_error("[appLoadAssets] Failed to load OpentCV wall images");

    // Load homography matrices from XML files
    for (const int &mon_ind : I.proj_mon_vec) // for each monitor index
    {
        for (int cal_i = 0; cal_i < N_CAL_MODES; ++cal_i)
        {
            CalibrationMode _CAL_MODE = static_cast<CalibrationMode>(cal_i);

            // Specify number of rows/cols to loop through based on active calibration mode
            int grid_size = (_CAL_MODE == WALLS_LEFT || _CAL_MODE == WALLS_MIDDLE || _CAL_MODE == WALLS_RIGHT) ? MAZE_SIZE : 1;

            // Iterate through the maze grid rows
            for (int gr_i = 0; gr_i < grid_size; ++gr_i)
            {
                for (int gc_i = 0; gc_i < grid_size; ++gc_i)
                {
                    if (xmlLoadHMAT(mon_ind, _CAL_MODE, gr_i, gc_i, HMAT_ARR_VEC[mon_ind][_CAL_MODE][gr_i][gc_i]) < 0)
                        throw std::runtime_error("[appMainLoop] Error returned from xmlLoadHMAT");
                }
            }
        }
    }

    ROS_INFO("[appLoadAssets] OpentCV mat images and 3D homography matrix array loaded successfully");
}

void appInitVariables()
{
    winOffsetVec.clear();              // Clear any existing elements
    winOffsetVec.reserve(N.projector); // Reserve memory for efficiency

    for (int win_ind = 0; win_ind < N.projector; ++win_ind)
    {
        int offsetValue = static_cast<int>(500.0f * (win_ind + 0.1f) * 0.2f);
        winOffsetVec.emplace_back(offsetValue, offsetValue);
    }

    ROS_INFO("[appInitVariables] Variables initialized succesfully");
}

void appInitOpenGL()
{

    // Initialize GLFW and OpenGL settings
    if (MazeRenderContext::SetupGraphicsLibraries(N.monitor) < 0)
        throw std::runtime_error("[appInitOpenGL] Failed to initialize graphics");

    // Check if expected monitors exceed available monitors
    if (I.proj_mon_vec.back() >= N.monitor) // compare last entry
        throw std::runtime_error("[appInitOpenGL] Monitor index exceeds available monitors");

    // Initialize OpenGL for each projector
    int win_ind = 0;
    for (auto &projCtx : PROJ_CTX_VEC)
    {

        // Initialze render context for each projector
        if (projCtx.initWindowContext(win_ind++, I.starting_monitor, WINDOW_WIDTH_PXL, WINDOW_HEIGHT_PXL, callbackKeyBinding) < 0)
            throw std::runtime_error("[appInitOpenGL] Failed to initialize render context");

        // Initialize OpenGL wall image objects
        if (initWallRenderObjects(QUAD_GL_VERTICES, sizeof(QUAD_GL_VERTICES),
                                  QUAD_GL_INDICES, sizeof(QUAD_GL_INDICES),
                                  projCtx) < 0)
            throw std::runtime_error("[appInitOpenGL] Failed to initialize opengl wall image objects");

        // Set all projectors to the starting monitor and include xy offset
        if (projCtx.changeWindowDisplayMode(I.starting_monitor, F.fullscreen_mode, winOffsetVec[projCtx.windowInd]) < 0)
            throw std::runtime_error("[appInitOpenGL] Window[" + std::to_string(projCtx.windowInd) + "]: Failed Initial update of window monitor mode");

        // Create the shader program for wall image rendering
        if (projCtx.compileAndLinkShaders(QUAD_GL_VERTEX_SOURCE, QUAD_GL_FRAGMENT_SOURCE) < 0)
            throw std::runtime_error("[appInitOpenGL] Window[" + std::to_string(projCtx.windowInd) + "]: Failed to compile and link wall shader");

        // Initialize the CircleRenderer class object for rat masking
        ratMaskCircRend.initializeCircleAttributes(
            rmPosition,      // position
            rmMakerRadius,   // radius
            rmRGB,           // color
            rmRenderSegments // segments
        );

        ROS_INFO("[appInitOpenGL] OpenGL initialized: Window[%d] Monitor[%d]", projCtx.windowInd, projCtx.monitorInd);
    }

    ROS_INFO("[appInitOpenGL] OpenGL contexts and objects Initialized succesfully");
}

void appMainLoop()
{
    int status = 0;
    while (status == 0)
    {
        // --------------- Check Kayboard Callback Flags ---------------

        // Update the window monitor and mode
        if (F.change_window_mode)
        {
            for (auto &projCtx : PROJ_CTX_VEC)
            {
                int mon_ind = F.windows_set_to_proj ? I.proj_mon_vec[projCtx.windowInd] : I.starting_monitor;
                if (projCtx.changeWindowDisplayMode(mon_ind, F.fullscreen_mode) < 0)
                    throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from MazeRenderContext::changeWindowDisplayMode");
            }
        }

        // Recompute wall parameters and update wall image texture
        if (F.update_textures)
        {
            for (auto &projCtx : PROJ_CTX_VEC)
            {
                // Initialize wall image texture
                if (updateTexture(I.proj_mon_vec[projCtx.windowInd], wallImgMatVec, floorImgMatVec, HMAT_ARR_VEC, projCtx))
                    throw std::runtime_error("[appInitOpenGL] Window[" + std::to_string(projCtx.windowInd) + "]: Failed to initialize wall texture");
            }
        }

        // Reset keybinding flags
        F.change_window_mode = false;
        F.update_textures = false;

        // --------------- Handle Image Processing for Next Frame ---------------

        for (auto &projCtx : PROJ_CTX_VEC)
        {

            // Prepare the frame for rendering (clear the back buffer)
            if (projCtx.initWindowForDrawing() < 0)
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from MazeRenderContext::initWindowForDrawing");

            // Make sure winsow always stays on top in fullscreen mode
            if (projCtx.forceWindowStackOrder(F.fullscreen_mode) < 0)
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from MazeRenderContext::forceWindowStackOrder");

            // Draw/update wall images
            if (projCtx.drawTexture() < 0)
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from drawTexture");

            // Swap buffers and poll events
            if (projCtx.bufferSwapPoll() < 0)
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from MazeRenderContext::bufferSwapPoll");

            // Check if ROS shutdown
            if (!ros::ok())
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Unextpected ROS shutdown");

            // Check for exit
            status = projCtx.checkExitRequest();
            if (status > 0)
                break;
            else if (status < 0)
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from MazeRenderContext::checkExitRequest");
        }
    }

    return;

    // Check which condition caused the loop to exit
    if (status == 1)
        ROS_INFO("[appMainLoop] Loop Terminated:  GLFW window should close");
    else if (status == 2)
        ROS_INFO("[appMainLoop] Loop Terminated:  Escape key was pressed");
    else
        ROS_INFO("[appMainLoop] Loop Terminated:  Reason unknown");
}

void appCleanup()
{
    ROS_INFO("SHUTTING DOWN");

    // Clean up OpenGL wall image objects for each window
    for (int proj_i = 0; proj_i < N.projector; ++proj_i)
    {
        if (PROJ_CTX_VEC[proj_i].cleanupContext(true) != 0)
            ROS_WARN("[appCleanup] Error during cleanup of MazeRenderContext: Window[%d] Monitor[%d]",
                     proj_i, PROJ_CTX_VEC[proj_i].monitorInd);
        else
            ROS_INFO("[appCleanup] MazeRenderContext instance cleaned up successfully: Window[%d] Monitor[%d]",
                     proj_i, PROJ_CTX_VEC[proj_i].monitorInd);
    }

    // Terminate graphics
    if (MazeRenderContext::CleanupGraphicsLibraries() < 0)
        ROS_WARN("[appCleanup] Failed to terminate GLFW library");
    else
        ROS_INFO("[appCleanup] GLFW library terminated successfully");
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "projection_calibration", ros::init_options::AnonymousName);
    ros::NodeHandle n;
    ros::NodeHandle nh("~");

    try
    {
        appInitVariables();
        appLoadAssets();
        appInitOpenGL();
        appMainLoop();
    }
    catch (const std::exception &e)
    {
        ROS_ERROR("!!EXCEPTION CAUGHT!!: %s", e.what());
        void appCleanup();
        return -1;
    }
    return 0;
}
