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

        // ---------- Set/unset Fullscreen [F] ----------

        if (key == GLFW_KEY_F)
        {
            F.fullscreen_mode = !F.fullscreen_mode;
            F.change_window_mode = true;
        }

        // ---------- Check for change window monitor command [M] ----------

        if (key == GLFW_KEY_M)
        {
            F.windows_set_to_proj = !F.windows_set_to_proj;
            F.change_window_mode = true;
        }

        // ---------- Force Window to Top of UI Stack [T] ----------

        if (key == GLFW_KEY_T)
        {
            F.force_window_focus = true;
        }

        // ---------- Change wall configuration [0-8] ----------
        if (RC.last_projection_cmd < PROJ_WALL_IMAGE_CFG_4D_VEC.size())
        {
            // Update the image configuration index
            I.wall_image_cfg = RC.last_projection_cmd;
            // Set the flag to update the textures
            F.update_textures = true;
        }
    }
}

void callbackCmdROS(const std_msgs::Int32::ConstPtr &msg, ROSComm *out_RC)
{
    // Update the last received command
    out_RC->last_projection_cmd = msg->data;
    out_RC->is_message_received = true;
    ROS_INFO("[callbackCmdROS] Received projection command: %d", out_RC->last_projection_cmd);
}

int initSubscriberROS(ROSComm &out_RC)
{
    // Check if node handle is initialized
    if (!out_RC.node_handle)
    {
        ROS_ERROR("[initSubscriberROS]Node handle is not initialized!");
        return -1;
    }

    // Initialize the subscriber using boost::bind
    out_RC.projection_cmd_sub = out_RC.node_handle->subscribe<std_msgs::Int32>(
        "projection_cmd", 10, boost::bind(&callbackCmdROS, _1, &out_RC));

    if (!out_RC.projection_cmd_sub)
    {
        ROS_ERROR("[initSubscriberROS]Failed to subscribe to 'projection_cmd' topic!");
        return -1;
    }

    return 0;
}

int procCmdROS(ROSComm &out_RC)
{
    // Check if node handle is initialized
    if (!ros::ok())
    {
        ROS_ERROR("[procCmdROS]ROS is no longer running!");
        return -1;
    }

    // Process a single round of callbacks for ROS messages
    ros::spinOnce();

    // Bail if no message received
    if (!out_RC.is_message_received)
        return 0;

    // Reset the flag
    out_RC.is_message_received = false;

    // Log the received command
    ROS_INFO("[procCmdROS] Processing received projection command: %d", out_RC.last_projection_cmd);

    // ---------- Monitor Mode Change Commmands ----------

    // Move monitor command [-1]
    if (out_RC.last_projection_cmd == -1)
    {
        F.windows_set_to_proj = !F.windows_set_to_proj;
        F.change_window_mode = true;
    }

    // Set/unset Fullscreen [-2] ----------
    else if (out_RC.last_projection_cmd == -2)
    {
        F.fullscreen_mode = !F.fullscreen_mode;
        F.change_window_mode = true;
    }

    // Force window to top [-3] ----------
    else if (out_RC.last_projection_cmd == -3)
    {
        F.force_window_focus = true;
    }

    // ---------- Change image configuraton ----------

    else if (out_RC.last_projection_cmd >= 0 && out_RC.last_projection_cmd <= 9)
    {
        if (out_RC.last_projection_cmd >= PROJ_WALL_IMAGE_CFG_4D_VEC.size())
        {
            ROS_WARN("[procCmdROS] Received image configuration index exceeds image configurations vector");
            return 0;
        }
        // Update the image configuration index
        I.wall_image_cfg = out_RC.last_projection_cmd;
        // Set the flag to update the textures
        F.update_textures = true;
    }

    else
    {
        ROS_WARN("[procCmdROS] Received invalid projection command: %d", out_RC.last_projection_cmd);
    }

    return 0;
}

void simulateRatMovement(float move_step, float max_turn_angle, RatTracker &out_RT)
{
    // Track marker angle
    static float marker_angle = 0.0f;

    // Lambda function to keep the rat within the enclosure and turn when hitting the wall
    auto keepWithinBoundsAndTurn = [&](cv::Point2f &position)
    {
        cv::Point2f original_position = position;
        position.x = std::min(std::max(position.x, 0.0f), GLB_MAZE_WIDTH_HEIGHT_CM);
        position.y = std::min(std::max(position.y, 0.0f), GLB_MAZE_WIDTH_HEIGHT_CM);

        // If position changed (rat hits the wall), rotate randomly up to 180 degrees
        if (position != original_position)
        {
            marker_angle += rand() % 181 - 90; // Random turn between -90 and +90 degrees
        }
    };

    // Randomly decide to change direction
    if (rand() % 10 == 0)
    { // 10% chance to change direction
        marker_angle += (rand() % static_cast<int>(2 * max_turn_angle)) - max_turn_angle;
    }

    // Calculate new position
    float radian_angle = marker_angle * PI / 180.0f;
    out_RT.marker_position.x += move_step * cos(radian_angle);
    out_RT.marker_position.y += move_step * sin(radian_angle);

    // Keep the rat within the enclosure and turn if hitting the wall
    keepWithinBoundsAndTurn(out_RT.marker_position);
}

void addImageConfiguration(const std::string &direction, int left_shape_ind, int right_shape_ind,
                           std::vector<ProjWallImageCfg4D> &out_PROJ_WALL_IMAGE_CFG_4D_VEC)
{
    // Initialize a new 4D array with all zeros
    ProjWallImageCfg4D new_array = {};

    // If blank is true, skip setting specific values and just return the array of zeros
    if (direction != "blank")
    {
        // Determine projectors involved relative to direction
        int proj_primary_ind = -1;    // Projector projecting onto both walls
        int proj_left_wall_ind = -1;  // Projector projecting onto the left wall
        int proj_right_wall_ind = -1; // Projector projecting onto the right wall

        if (direction == "east")
        {
            proj_primary_ind = 0;    // P0: East facing
            proj_left_wall_ind = 3;  // P3: North facing
            proj_right_wall_ind = 1; // P1: South facing
        }
        else if (direction == "south")
        {
            proj_primary_ind = 1;    // P1: South facing
            proj_left_wall_ind = 0;  // P0: East facing
            proj_right_wall_ind = 2; // P2: West facing
        }
        else if (direction == "west")
        {
            proj_primary_ind = 2;    // P2: West facing
            proj_left_wall_ind = 1;  // P1: South facing
            proj_right_wall_ind = 3; // P3: North facing
        }
        else if (direction == "north")
        {
            proj_primary_ind = 3;    // P3: North facing
            proj_left_wall_ind = 2;  // P2: West facing
            proj_right_wall_ind = 0; // P0: East facing
        }

        // Get the center index based on the maze grid size
        int center_ind = (GLB_MAZE_SIZE - 1) / 2;

        // Set the left and right values for the primary projectors left and right wall indices
        new_array[proj_primary_ind][center_ind][center_ind][0] = left_shape_ind;  // Center/middle left
        new_array[proj_primary_ind][center_ind][center_ind][2] = right_shape_ind; // Center/middle right

        // Set the values for the left and right projector, 'left' corresponds to the right value and vice versa
        new_array[proj_left_wall_ind][center_ind][center_ind][2] = left_shape_ind;   // Center/middle right
        new_array[proj_right_wall_ind][center_ind][center_ind][0] = right_shape_ind; // Center/middle left

        // if (GLB_DO_VERBOSE_DEBUG)
        ROS_INFO("[addImageConfiguration] Added image configuration for direction %s: Projectors Primary[%d] Left:%d Right:%d]",
                 direction.c_str(), proj_primary_ind, proj_left_wall_ind, proj_right_wall_ind);

        if (GLB_DO_VERBOSE_DEBUG)
            dbLogProjWallImageCfg4D(new_array);
    }

    // Add the new configuration to the vector
    out_PROJ_WALL_IMAGE_CFG_4D_VEC.push_back(new_array);
}

void populateMazeVertNdcVec(int proj_ind, std::vector<cv::Point2f> &maze_vert_cm_vec)
{
    // Lambda function for circular shift
    auto circShift = [](const std::vector<cv::Point2f> &vec, int shift) -> std::vector<cv::Point2f>
    {
        std::vector<cv::Point2f> shifted_vec(vec.size());
        int n = static_cast<int>(vec.size());
        for (int v_i = 0; v_i < n; ++v_i)
        {
            shifted_vec[v_i] = vec[(v_i + shift + n) % n];
        }
        return shifted_vec;
    };

    // Template vertices
    const std::vector<cv::Point2f> template_maze_vert_cm_vec = {
        cv::Point2f(0, GLB_MAZE_WIDTH_HEIGHT_CM),
        cv::Point2f(GLB_MAZE_WIDTH_HEIGHT_CM, GLB_MAZE_WIDTH_HEIGHT_CM),
        cv::Point2f(GLB_MAZE_WIDTH_HEIGHT_CM, 0.0),
        cv::Point2f(0.0, 0.0)};

    // Apply circular shift based on the given projector
    switch (proj_ind)
    {
    case 0: // Circular shift left by 1
        maze_vert_cm_vec = circShift(template_maze_vert_cm_vec, -1);
        break;
    case 1: // No shift
        maze_vert_cm_vec = template_maze_vert_cm_vec;
        break;
    case 2: // Circular shift right by 1
        maze_vert_cm_vec = circShift(template_maze_vert_cm_vec, 1);
        break;
    case 3: // Circular shift right by 2
        maze_vert_cm_vec = circShift(template_maze_vert_cm_vec, 2);
        break;
    default:
        break;
    }
}

int updateTexture(
    const std::vector<cv::Mat> &_wallImgMatVec,
    const std::vector<cv::Mat> &_floorImgMatVec,
    const ProjWallImageCfg4D &_PROJ_WALL_IMAGE_CFG_3D,
    const ProjFloorImageCfg1D &_PROJ_FLOOR_IMAGE_CFG_1D,
    const std::array<std::array<std::array<std::array<cv::Mat, GLB_MAZE_SIZE>, GLB_MAZE_SIZE>, N_CAL_MODES>, 4> &_HMAT_ARR,
    MazeRenderContext &out_projCtx)
{
    // Initialize the image to be used as the texture
    cv::Mat img_merge = cv::Mat::zeros(GLB_MONITOR_HEIGHT_PXL, GLB_MONITOR_WIDTH_PXL, CV_8UC4);

    // Iterate through through calibration modes in descending order so floor is drawn first
    for (int cal_i = N_CAL_MODES - 1; cal_i >= 0; --cal_i)
    {
        CalibrationMode _CAL_MODE = static_cast<CalibrationMode>(cal_i);

        // Specify number of rows/cols to loop through
        int grid_size = (_CAL_MODE == WALLS_LEFT || _CAL_MODE == WALLS_MIDDLE || _CAL_MODE == WALLS_RIGHT) ? GLB_MAZE_SIZE : 1;

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
                    int img_ind = _PROJ_WALL_IMAGE_CFG_3D[out_projCtx.windowInd][gr_i][gc_i][_CAL_MODE];
                    if (_wallImgMatVec[img_ind].empty())
                    {
                        ROS_ERROR("[updateTexture] Stored OpenCV wall image is empty: Projector[%d] Wall[%d][%d] Calibration[%d] Image[%d]",
                                  out_projCtx.windowInd, gr_i, gc_i, _CAL_MODE, img_ind);
                        return -1;
                    }
                    else
                    {
                        _wallImgMatVec[img_ind].copyTo(img_copy);
                    }
                }
                else
                {
                    int img_ind = _PROJ_FLOOR_IMAGE_CFG_1D[out_projCtx.windowInd];
                    if (_floorImgMatVec[img_ind].empty())
                    {
                        ROS_ERROR("[updateTexture] Stored OpenCV floor image is empty: Projector[%d] Wall[%d][%d] Calibration[%d] Image[%d]",
                                  out_projCtx.windowInd, gr_i, gc_i, _CAL_MODE, img_ind);
                        return -1;
                    }
                    else
                    {
                        _floorImgMatVec[img_ind].copyTo(img_copy);
                    }
                }

                // Get homography matrix for this wall
                cv::Mat H = _HMAT_ARR[out_projCtx.windowInd][_CAL_MODE][gr_i][gc_i];

                // Warp Perspective
                cv::Mat img_warp;
                if (warpImgMat(img_copy, H, img_warp) < 0)
                {
                    ROS_ERROR("[updateTexture] Warp image error: Projector[%d] Wall[%d][%d] Calibration[%d]",
                              out_projCtx.windowInd, gr_i, gc_i, _CAL_MODE);
                    return -1;
                }

                // Merge the warped image with the final image
                if (mergeImgMat(img_warp, img_merge) < 0)
                    return -1;
            }
        }
    }

    // Load the new texture and return status
    if (out_projCtx.loadMatTexture(img_merge) < 0)
    {
        ROS_ERROR("[updateTexture] Failed to load texture");
        return -1;
    }

    return 0;
}

int drawRatMask(
    const RatTracker &_RT,
    CircleRenderer &out_rmCircRend)
{
    // Setup the CircleRenderer class shaders
    if (CircleRenderer::SetupShader() < 0)
        return -1;

    // Set the marker position
    out_rmCircRend.setPosition(_RT.marker_position);

    // Recompute the marker parameters
    if (out_rmCircRend.updateCircleObject(true) < 0)
        return -1;

    // Draw the marker
    if (out_rmCircRend.draw() < 0)
        return -1;

    // Unset the shader program
    if (CircleRenderer::UnsetShader() < 0)
        return -1;

    // Return GL status
    return 0;
}

void appInitROS(int argc, char **argv, ROSComm &out_RC)
{
    // Initialize ROS
    ros::init(argc, argv, "projection_calibration", ros::init_options::AnonymousName);
    if (!ros::master::check())
        throw std::runtime_error("[appInitROS] Failed initialzie ROS: ROS master is not running");

    // Initialize NodeHandle inside RC
    RC.node_handle = std::make_unique<ros::NodeHandle>();

    // Initialize the ros::Rate object with a specific rate, e.g., 100 Hz
    out_RC.loop_rate = std::make_unique<ros::Rate>(100); // 100 Hz

    // Initialize the subscriber
    if (initSubscriberROS(out_RC) < 0)
        throw std::runtime_error("[appInitROS] Failed to initialize ROS subscriber");

    ROS_INFO("[appInitROS] Finished initializing ROS successfully");
}

void appLoadAssets()
{
    // ---------- Load Images with OpenCV ----------
    if (loadImgMat(fiImgPathWallVec, wallImgMatVec) < 0)
        throw std::runtime_error("[appLoadAssets] Failed to load OpentCV wall images");
    if (loadImgMat(fiImgPathFloorVec, floorImgMatVec) < 0)
        throw std::runtime_error("[appLoadAssets] Failed to load OpentCV wall images");

    // ---------- Load Wall and Floor Homography Matrices from XML ----------
    for (int proj_ind = 0; proj_ind < N.projector; ++proj_ind) // for each projector
    {
        for (int cal_i = 0; cal_i < N_CAL_MODES; ++cal_i)
        {
            CalibrationMode _CAL_MODE = static_cast<CalibrationMode>(cal_i);

            // Specify number of rows/cols to loop through based on active calibration mode
            int grid_size = (_CAL_MODE == WALLS_LEFT || _CAL_MODE == WALLS_MIDDLE || _CAL_MODE == WALLS_RIGHT) ? GLB_MAZE_SIZE : 1;

            // Iterate through the maze grid rows
            for (int gr_i = 0; gr_i < grid_size; ++gr_i)
            {
                for (int gc_i = 0; gc_i < grid_size; ++gc_i)
                {
                    // Load the homography matrix from XML
                    if (xmlLoadHMAT(proj_ind, _CAL_MODE, gr_i, gc_i, HMAT_ARR[proj_ind][_CAL_MODE][gr_i][gc_i]) < 0)
                        throw std::runtime_error("[appLoadAssets] Error returned from: xmlLoadHMAT");
                }
            }
        }
    }

    // ---------- Load Maze Boundary Vertices ----------
    for (int proj_ind = 0; proj_ind < N.projector; ++proj_ind) // for each projector
    {
        std::vector<cv::Point2f> maze_vert_ndc_vec(4);
        std::vector<cv::Point2f> maze_vert_cm_vec(4);

        // Load the maze vertices from XML
        if (xmlLoadVertices(proj_ind, maze_vert_ndc_vec) < 0)
            throw std::runtime_error("[appLoadAssets] Error returned from: xmlLoadVertices");

        // Compute the rotated maze vertices in centimeter units
        populateMazeVertNdcVec(proj_ind, maze_vert_cm_vec);

        // Compute the homography matrix for warping the rat mask marker from maze cm to ndc space for each projector
        cv::Mat H;
        if (computeHomographyMatrix(maze_vert_cm_vec, maze_vert_ndc_vec, H))
            throw std::runtime_error("[appLoadAssets] Projector[" + std::to_string(proj_ind) + "]: Invalid homography matrix for rat mask image");

        // Store the homography matrix
        HMAT_CM_TO_NDC_ARR[proj_ind] = H;
    }

    ROS_INFO("[appLoadAssets] Finished loading variables successfully");
}

void appInitVariables()
{
    // ---------- Intialize the Window Offset Vector ---------
    winOffsetVec.clear();              // Clear any existing elements
    winOffsetVec.reserve(N.projector); // Reserve memory for efficiency
    for (int mon_ind = 0; mon_ind < N.projector; ++mon_ind)
    {
        // Calculate x and y offsets based on the monitor resolution
        int x_offset = mon_ind * (GLB_MONITOR_WIDTH_PXL / N.projector) * 0.9f;
        int y_offset = mon_ind * (GLB_MONITOR_HEIGHT_PXL / N.projector) * 0.9f;
        winOffsetVec.emplace_back(x_offset, y_offset);
    }

    // ---------- Initialize Wall Image Configuration Data (Afsoon) ----------

    // Specify the two shapes
    int shape1_ind = 2; // Circle
    int shape2_ind = 3; // Triangle

    // Blank choice point;
    addImageConfiguration("blank", shape1_ind, shape2_ind, PROJ_WALL_IMAGE_CFG_4D_VEC); // Index[0]

    // East facing choice point;
    addImageConfiguration("east", shape1_ind, shape2_ind, PROJ_WALL_IMAGE_CFG_4D_VEC); // Index[1], Left: shape 1, Right: shape 2
    addImageConfiguration("east", shape2_ind, shape1_ind, PROJ_WALL_IMAGE_CFG_4D_VEC); // Index[2], Left: shape 2, Right: shape 1

    // South facing choice point;
    addImageConfiguration("south", shape1_ind, shape2_ind, PROJ_WALL_IMAGE_CFG_4D_VEC); // Index[3], Left: shape 1, Right: shape 2
    addImageConfiguration("south", shape2_ind, shape1_ind, PROJ_WALL_IMAGE_CFG_4D_VEC); // Index[4], Left: shape 2, Right: shape 1

    // West facing choice point;
    addImageConfiguration("west", shape1_ind, shape2_ind, PROJ_WALL_IMAGE_CFG_4D_VEC); // Index[5], eft: shape 1, Right: shape 2
    addImageConfiguration("west", shape2_ind, shape1_ind, PROJ_WALL_IMAGE_CFG_4D_VEC); // Index[6], Left: shape 2, Right: shape 1

    // North facing choice point;
    addImageConfiguration("north", shape1_ind, shape2_ind, PROJ_WALL_IMAGE_CFG_4D_VEC); // Index[7], Left: shape 1, Right: shape 2
    addImageConfiguration("north", shape2_ind, shape1_ind, PROJ_WALL_IMAGE_CFG_4D_VEC); // Index[8], Left: shape 2, Right: shape 1

    ROS_INFO("[appInitVariables] Finished initializing variables successfully");
}

void appInitOpenGL()
{

    // Initialize GLFW and OpenGL settings and get number of monitors on the system
    if (MazeRenderContext::SetupGraphicsLibraries(N.monitor) < 0)
        throw std::runtime_error("[appInitOpenGL] Failed to initialize graphics");

    // Check if expected monitors exceed available monitors
    if (I.proj_mon_vec.back() >= N.monitor) // compare last entry
        throw std::runtime_error("[appInitOpenGL] Monitor index exceeds available monitors");

    // Initialize OpenGL for each projector
    for (int proj_ind = 0; proj_ind < N.projector; ++proj_ind)
    {
        // Start on the default screen
        int mon_ind = I.starting_monitor;

        // Initialze render context for each projector
        if (PROJ_CTX_VEC[proj_ind].initWindowContext(proj_ind, mon_ind, GLB_MONITOR_WIDTH_PXL, GLB_MONITOR_HEIGHT_PXL, callbackKeyBinding) < 0)
            throw std::runtime_error("[appInitOpenGL] Failed to initialize render context");

        // Initialize OpenGL wall image objects
        if (PROJ_CTX_VEC[proj_ind].initRenderObjects(GLB_QUAD_GL_VERTICES, sizeof(GLB_QUAD_GL_VERTICES), GLB_QUAD_GL_INDICES, sizeof(GLB_QUAD_GL_INDICES)) < 0)
            throw std::runtime_error("[appInitOpenGL] Failed to initialize opengl wall image objects");

        // Create the shader program for wall image rendering
        if (PROJ_CTX_VEC[proj_ind].compileAndLinkShaders(GLB_QUAD_GL_VERTEX_SOURCE, GLB_QUAD_GL_FRAGMENT_SOURCE) < 0)
            throw std::runtime_error("[appInitOpenGL] Window[" + std::to_string(PROJ_CTX_VEC[proj_ind].windowInd) + "]: Failed to compile and link wall shader");

        // Create the shader program for CircleRenderer class rat mask rendering
        if (CircleRenderer::CompileAndLinkCircleShaders(1.0) < 0)
            throw std::runtime_error("[appInitOpenGL] Failed to compile and link circlerenderer class shader");

        // Set all projectors to the starting monitor and include xy offset
        if (PROJ_CTX_VEC[proj_ind].changeWindowDisplayMode(mon_ind, F.fullscreen_mode, winOffsetVec[PROJ_CTX_VEC[proj_ind].windowInd]) < 0)
            throw std::runtime_error("[appInitOpenGL] Window[" + std::to_string(PROJ_CTX_VEC[proj_ind].windowInd) + "]: Failed Initial update of window monitor mode");

        // Initialize the CircleRenderer class object for rat masking
        if (RM_CIRCREND_ARR[proj_ind].initializeCircleObject(
                RT.marker_position,          // position
                RT.marker_radius,            // radius
                RT.marker_rgb,               // color
                RT.marker_segments,          // segments
                HMAT_CM_TO_NDC_ARR[proj_ind] // homography matrix
                ) < 0)
            throw std::runtime_error("[appInitOpenGL] Failed to initialize CircleRenderer class object");

        ROS_INFO("[appInitOpenGL] OpenGL initialized: Projector[%d] Window[%d] Monitor[%d]", proj_ind, PROJ_CTX_VEC[proj_ind].windowInd, PROJ_CTX_VEC[proj_ind].monitorInd);
    }

    ROS_INFO("[appInitOpenGL] OpenGL contexts and objects Initialized succesfully");
}

void appMainLoop()
{
    int status = 0;
    while (status == 0)
    {
        // --------------- Check State Flags ---------------

        // Update the window monitor and mode
        if (F.change_window_mode)
        {
            for (auto &projCtx : PROJ_CTX_VEC)
            {
                int mon_ind = F.windows_set_to_proj ? I.proj_mon_vec[projCtx.windowInd] : I.starting_monitor;
                if (projCtx.changeWindowDisplayMode(mon_ind, F.fullscreen_mode, winOffsetVec[projCtx.windowInd]) < 0)
                    throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from: MazeRenderContext::changeWindowDisplayMode");
            }
        }

        // Force to windows so thay are on top
        if (F.force_window_focus)
        {
            for (auto &projCtx : PROJ_CTX_VEC)
            {
                if (projCtx.forceWindowFocus() < 0)
                    throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from: MazeRenderContext::forceWindowFocus");
            }
        }

        // Recompute wall parameters and update wall image texture
        if (F.update_textures)
        {
            for (auto &projCtx : PROJ_CTX_VEC)
            {
                // Initialize wall image texture
                if (updateTexture(wallImgMatVec, floorImgMatVec, PROJ_WALL_IMAGE_CFG_4D_VEC[I.wall_image_cfg], PROJ_FLOOR_IMAGE_CFG_1D, HMAT_ARR, projCtx))
                    throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Failed to initialize wall texture");
            }
        }

        // Reset keybinding flags
        F.change_window_mode = false;
        F.update_textures = false;
        F.force_window_focus = false;

        // --------------- Handle Image Processing for Next Frame ---------------

        for (auto &projCtx : PROJ_CTX_VEC)
        {
            // Prepare the frame for rendering (clear the back buffer)
            if (projCtx.initWindowForDrawing() < 0)
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from: MazeRenderContext::initWindowForDrawing");

            // Draw/update wall images
            if (projCtx.drawTexture() < 0)
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from: drawTexture");

            // TEMP Simulate rat movement for testing (set color to red)
            simulateRatMovement(0.5f, 45.0f, RT);

            // Draw/update rat mask marker
            if (drawRatMask(RT, RM_CIRCREND_ARR[projCtx.windowInd]) < 0)
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from: drawRatMask");

            // Swap buffers and poll events
            if (projCtx.bufferSwapPoll() < 0)
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from: MazeRenderContext::bufferSwapPoll");

            // Check if ROS shutdown
            if (!ros::ok())
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Unexpected ROS shutdown");

            // Check for exit
            status = projCtx.checkExitRequest();
            if (status > 0)
                break;
            else if (status < 0)
                throw std::runtime_error("[appMainLoop] Window[" + std::to_string(projCtx.windowInd) + "]: Error returned from: MazeRenderContext::checkExitRequest");
        }

        // --------------- Handle ROS Messages and Opperations ---------------

        // Process ROS messages
        if (procCmdROS(RC) < 0)
            throw std::runtime_error("[appMainLoop] Error returned from: procCmdROS");

        // Sleep to maintain the loop rate
        RC.loop_rate->sleep();
    }

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
    for (int proj_ind = 0; proj_ind < N.projector; ++proj_ind)
    {
        if (PROJ_CTX_VEC[proj_ind].cleanupContext(true) != 0)
            ROS_WARN("[appCleanup] Error during cleanup of MazeRenderContext: Projector[%d] Window[%d] Monitor[%d]",
                     proj_ind, PROJ_CTX_VEC[proj_ind].windowInd, PROJ_CTX_VEC[proj_ind].monitorInd);
        else
            ROS_INFO("[appCleanup] MazeRenderContext instance cleaned up successfully: Projector[%d] Window[%d] Monitor[%d]",
                     proj_ind, PROJ_CTX_VEC[proj_ind].windowInd, PROJ_CTX_VEC[proj_ind].monitorInd);
    }

    // Terminate graphics
    if (MazeRenderContext::CleanupGraphicsLibraries() < 0)
        ROS_WARN("[appCleanup] Failed to terminate GLFW library");
    else
        ROS_INFO("[appCleanup] GLFW library terminated successfully");
}

int main(int argc, char **argv)
{
    try
    {
        appInitROS(argc, argv, RC);
        appLoadAssets();
        appInitVariables();
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
