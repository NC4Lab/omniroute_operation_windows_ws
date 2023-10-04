/**
 * @file projection_calibration.cpp
 * @brief This file contains the implementation for the projection calibration.
 */

//============= INCLUDE ================
#include "projection_calibration.h"

/**
 * @brief Callback function for handling key bindings.
 *
 * @param window Pointer to the GLFW window.
 * @param key The key that was pressed or released.
 * @param scancode The system-specific scancode of the key.
 * @param action GLFW_PRESS, GLFW_RELEASE, or GLFW_REPEAT.
 * @param mods Bit field describing which modifier keys were held down.
 *
 *  @ref: GLFW/glfw3.h for keybindings enum
 *
 * Key Bindings:
 * - [1-4]: Select target control point (Top-left, Top-right, Bottom-right, Bottom-left)
 * - [F1-F12]: Set image to image 1 to 12
 * - [A, D, S]: Change control point mode(position/translation, dimension/height, shear)
 * - [ENTER]: Save coordinates to XML
 * - [L]: Load coordinates from XML
 * - [F]: Fullscreen on second monitor
 * - [M]: Move window to next monitor
 * - [Arrow Keys]: Move selected control point or adjust dimension/shear
 */
void callbackKeyBinding(GLFWwindow *window, int key, int scancode, int action, int mods)
{

    glfwMakeContextCurrent(window);

    // _______________ ANY KEY RELEASE ACTION _______________

    if (action == GLFW_RELEASE)
    {
        // ---------- Target selector keys [1-4] ----------

        // Top-left control point
        if (key == GLFW_KEY_1)
        {
            cpSelected = 0;
        }

        // Top-right control point
        else if (key == GLFW_KEY_2)
        {
            cpSelected = 1;
        }

        // Bottom-right control point
        else if (key == GLFW_KEY_3)
        {
            cpSelected = 2;
        }

        // Bottom-left control point
        else if (key == GLFW_KEY_4)
        {
            cpSelected = 3;
        }

        // ---------- Image selector keys [F1-F12] ----------

        else if (key == GLFW_KEY_F1)
        {
            imageInd = 0;
        }
        else if (key == GLFW_KEY_F2)
        {
            imageInd = 1;
        }
        else if (key == GLFW_KEY_F3)
        {
            imageInd = 2;
        }
        else if (key == GLFW_KEY_F4)
        {
            imageInd = 3;
        }

        // ---------- Change mode keys [A, D, S] ----------

        // Control point position [up, down, left, right]
        else if (key == GLFW_KEY_A)
        {
            cpModMode = "position";
        }

        // Control point height [up, down]
        else if (key == GLFW_KEY_D)
        {
            cpModMode = "dimension";
        }

        // Control point shear [up, down]
        else if (key == GLFW_KEY_S)
        {
            cpModMode = "shear";
        }

        // ---------- XML Handling [ENTER, L] ----------

        // Save coordinates to XML
        else if (key == GLFW_KEY_ENTER)
        {
            ROS_INFO("save hit");
            saveCoordinatesXML();
        }

        // Load coordinates from XML
        else if (key == GLFW_KEY_L)
        {
            loadCoordinatesXML();
        }

        // ---------- Monitor handling [F, M] ----------

        // Set/unset Fullscreen
        else if (key == GLFW_KEY_F)
        {
            isFullScreen = !isFullScreen;
            changeWindowMonMode();
        }

        // Move the window to the other monitor
        else if (key == GLFW_KEY_M)
        {
            monitorInd = (monitorInd < monitorCount - 1) ? monitorInd + 1 : 0;
            changeWindowMonMode();
        }
    }

    // _______________ ANY KEY PRESS OR REPEAT ACTION _______________
    else if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {

        // ---------- Control point position change [LEFT, RIGHT, UP, DOWN] ----------
        if (cpModMode == "position")
        {

            // Listen for arrow key input to move selected control point
            if (key == GLFW_KEY_LEFT)
            {
                cpPositions[cpSelected][0] -= 0.05f;
            }
            else if (key == GLFW_KEY_RIGHT)
            {
                cpPositions[cpSelected][0] += 0.05f;
            }
            else if (key == GLFW_KEY_UP)
            {
                cpPositions[cpSelected][1] += 0.05f;
            }
            else if (key == GLFW_KEY_DOWN)
            {
                cpPositions[cpSelected][1] -= 0.05f;
            }
        }

        // ---------- Control point dimension/hight change [UP, DOWN] ----------
        if (cpModMode == "dimension")
        {
            if (key == GLFW_KEY_UP)
            {
                cpPositions[cpSelected][3] += 0.001f;
            }
            else if (key == GLFW_KEY_DOWN)
            {
                cpPositions[cpSelected][3] -= 0.001f;
            }
        }

        // ---------- Control point shear change [UP, DOWN] ----------
        if (cpModMode == "shear")
        {
            if (key == GLFW_KEY_UP)
            {
                cpPositions[cpSelected][4] += 0.05f;
            }
            else if (key == GLFW_KEY_DOWN)
            {
                cpPositions[cpSelected][4] -= 0.05f;
            }
        }
    }

    // ---------- Recompute homography matrix ----------
    computeHomography();
}

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
void callbackFrameBufferSize(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

/**
 * @brief Callback function for handling errors.
 *
 * This function is called whenever an error occurs in the GLFW context.
 * It logs the error message using ROS_ERROR.
 *
 * @param error The error code.
 * @param description The error description.
 */
static void callbackError(int error, const char *description)
{
    ROS_ERROR("Error: %s\n", description);
}

/**
 * @brief Draws a control point as a quadrilateral using OpenGL.
 *
 * This function uses OpenGL to draw a quadrilateral that represents a control point.
 * The control point is drawn in a clockwise direction, starting from the bottom-left corner.
 *
 * @param x The x-coordinate of the bottom-left corner of the control point.
 * @param y The y-coordinate of the bottom-left corner of the control point.
 * @param cp_width The width of the control point.
 * @param cp_height The height of the control point.
 */
void drawControlPoint(float x, float y, float cp_width, float cp_height)
{

    // Begin drawing a quadrilateral
    glBegin(GL_QUADS);

    // Set the color to green
    glColor3f(0.0f, 1.0f, 0.0f);

    // Define the vertices of the quadrilateral in a clockwise direction
    // starting from the bottom-left corner
    glVertex2f(x, y);                        // Bottom-left corner
    glVertex2f(x, y + cp_height);            // Top-left corner
    glVertex2f(x + cp_width, y + cp_height); // Top-right corner
    glVertex2f(x + cp_width, y);             // Bottom-right corner

    // End drawing
    glEnd();
}

void drawControlPointCircle(float x, float y, float radius)
{
    int segments = 100; // Number of segments to approximate a circle

    // Begin drawing a filled circle
    glBegin(GL_TRIANGLE_FAN);

    // Set the color to green
    glColor3f(0.0f, 1.0f, 0.0f);

    // Center of the circle
    glVertex2f(x, y);

    // Calculate and draw the vertices of the circle
    for (int i = 0; i <= segments; i++)
    {
        float theta = 2.0f * 3.1415926f * float(i) / float(segments);
        float px = x + radius * cosf(theta);
        float py = y + (radius * winAspectRatio) * sinf(theta);
        glVertex2f(px, py);
    }

    // End drawing
    glEnd();
}

/**
 * @brief Draws a textured wall using OpenGL.
 *
 * @param corners Vector of corner points for the wall.
 */
void drawWall(std::vector<cv::Point2f> img_vertices)
{
    // Start drawing a quadrilateral
    glBegin(GL_QUADS);

    // Set the color to white
    glColor3f(1.0f, 1.0f, 1.0f);

    // Set texture and vertex coordinates for each corner
    // Bottom-left corner
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(img_vertices[0].x, img_vertices[0].y);

    // Bottom-right corner
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(img_vertices[1].x, img_vertices[1].y);

    // Top-right corner
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(img_vertices[2].x, img_vertices[2].y);

    // Top-left corner
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(img_vertices[3].x, img_vertices[3].y);

    // End drawing
    glEnd();
}

/**
 * @brief Draws all the walls in the maze using OpenGL and OpenCV.
 *
 * This function iterates through the maze grid and draws each wall with
 * texture mapping and perspective warping. It uses control points to
 * determine the shear and height for each wall.
 */
void drawWallsAll()
{
    // Extract shear and height values from control points
    float height1 = cpPositions[0][3];
    float height3 = cpPositions[2][3];
    float height4 = cpPositions[3][3];
    float shear1 = cpPositions[0][4];
    float shear3 = cpPositions[2][4];
    float shear4 = cpPositions[3][4];

    // Enable OpenGL texture mapping
    glEnable(GL_TEXTURE_2D);

    // Iterate through the maze grid
    for (float i_wall = 0; i_wall < MAZE_SIZE; i_wall++)
    {
        // Bind and set texture image
        ilBindImage(imgTestIDs[imageInd]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ilGetInteger(IL_IMAGE_WIDTH),
                     ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGB,
                     GL_UNSIGNED_BYTE, ilGetData());

        // Iterate through each cell in the maze row
        for (float j_wall = 0; j_wall < MAZE_SIZE; j_wall++)
        {
            // Bind texture to framebuffer object
            glBindTexture(GL_TEXTURE_2D, fboTexture);

            // Calculate shear and height for the current wall
            float shear_val = shear4 + (i_wall / (MAZE_SIZE - 1)) * (shear3 - shear4) +
                              (j_wall / (MAZE_SIZE - 1)) * (shear1 - shear4);
            float height_val = height4 + (i_wall / (MAZE_SIZE - 1)) * (height3 - height4) +
                               (j_wall / (MAZE_SIZE - 1)) * (height1 - height4);

            // Create wall vertices
            std::vector<cv::Point2f> img_vertices = computeWallVertices(0.0f, 0.0f, wallWidth, height_val, shear_val);

            // Apply perspective warping to vertices
            for (auto &p : img_vertices)
            {
                // Update vertex positions based on shear and height
                p.x += i_wall * wallSpace;
                p.y += j_wall * wallSpace;

                // Apply homography matrix to warp perspective
                float data[] = {p.x, p.y, 1};
                cv::Mat ptMat(3, 1, CV_32F, data);
                H.convertTo(H, ptMat.type());
                ptMat = H * ptMat;
                ptMat /= ptMat.at<float>(2);

                // Update vertex coordinates
                p.x = ptMat.at<float>(0, 0);
                p.y = ptMat.at<float>(0, 1);
            }

            // Draw the wall
            drawWall(img_vertices);
        }
    }
}

void changeWindowMonMode()
{
    // Use modulo to loop back to the first monitor if we've reached the end
    monitor = monitors[monitorInd];

    if (monitor)
    {
        // Get the video mode of the selected monitor
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);

        // Set the window to full-screen mode on the current monitor
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

        if (!isFullScreen)
        {
            // Get the position of the current monitor
            int monitor_x, monitor_y;
            glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);

            // Set the window to windowed mode and position it on the current monitor
            glfwSetWindowMonitor(window, NULL, monitor_x + 100, monitor_y + 100, (int)(500.0f * winAspectRatio), 500, 0);
        }
        ROS_INFO("Moved window to monitor %d and set to %s", monitorInd + 1, isFullScreen ? "fullscreen" : "windowed");
    }
    else
    {
        ROS_WARN("Monitor not found. Could not change window mode or move monitor.");
    }
}

/**
 * @brief Creates a vector of points representing a rectangle with shear for each wall.
 *
 * This function generates a rectangle's corner points starting from the top-left corner
 * and going clockwise. The rectangle is defined by its top-left corner (x0, y0),
 * width, height, and a shear amount.
 *
 * @param x0 The x-coordinate of the top-left corner of the rectangle.
 * @param y0 The y-coordinate of the top-left corner of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @param shear_amount The amount of shear to apply to the rectangle.
 *
 * @return std::vector<cv::Point2f> A vector of 4 points representing the corners of the rectangle.
 */
std::vector<cv::Point2f> computeWallVertices(float x0, float y0, float width, float height, float shear_amount)
{
    std::vector<cv::Point2f> rect_vertices;

    // Top-left corner after applying shear
    rect_vertices.push_back(cv::Point2f(x0 + height * shear_amount, y0 + height));

    // Top-right corner after applying shear
    rect_vertices.push_back(cv::Point2f(x0 + height * shear_amount + width, y0 + height));

    // Bottom-right corner
    rect_vertices.push_back(cv::Point2f(x0 + width, y0));

    // Bottom-left corner
    rect_vertices.push_back(cv::Point2f(x0, y0));

    return rect_vertices;
}

void computeHomography()
{
    // Get the corner/vertex values for each of the control points
    std::vector<cv::Point2f> cp_vertices;
    cp_vertices.push_back(cv::Point2f(cpPositions[0][0], cpPositions[0][1]));
    cp_vertices.push_back(cv::Point2f(cpPositions[1][0], cpPositions[1][1]));
    cp_vertices.push_back(cv::Point2f(cpPositions[2][0], cpPositions[2][1]));
    cp_vertices.push_back(cv::Point2f(cpPositions[3][0], cpPositions[3][1]));

    // Get the corner/vertex values for each of the wall images
    std::vector<cv::Point2f> img_vertices;
    img_vertices = computeWallVertices(0.0f, 0.0f, (float(MAZE_SIZE) - 1) * wallSpace, (float(MAZE_SIZE) - 1) * wallSpace, 0);

    // Compute the homography matrix
    H = findHomography(img_vertices, cp_vertices);
}

void loadCoordinatesXML()
{
    pugi::xml_document doc;
    if (!doc.load_file(configPath.c_str()))
    {
        ROS_ERROR("Failed to load XML file.");
        return;
    }

    // Retrieve cpPositions
    std::vector<std::vector<float>> cpPositions2;
    pugi::xml_node cpPositionsNode = doc.child("config").child("cpPositions");
    for (pugi::xml_node rowNode = cpPositionsNode.child("Row"); rowNode; rowNode = rowNode.next_sibling("Row"))
    {
        std::vector<float> row;
        for (pugi::xml_node cellNode = rowNode.child("Cell"); cellNode; cellNode = cellNode.next_sibling("Cell"))
        {
            float value = std::stof(cellNode.child_value());
            row.push_back(value);
        }
        cpPositions2.push_back(row);
    }

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            cpPositions[i][j] = cpPositions2[i][j];
        }
    }

    // Retrieve H
    std::vector<std::vector<float>> H2;
    pugi::xml_node HNode = doc.child("config").child("H");
    for (pugi::xml_node rowNode = HNode.child("Row"); rowNode; rowNode = rowNode.next_sibling("Row"))
    {
        std::vector<float> row;
        for (pugi::xml_node cellNode = rowNode.child("Cell"); cellNode; cellNode = cellNode.next_sibling("Cell"))
        {
            float value = std::stof(cellNode.child_value());
            row.push_back(value);
        }
        H2.push_back(row);
    }
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            H.at<float>(i, j) = H2[i][j];
        }
    }
}

/**
 * @brief Saves the control point positions and homography matrix to an XML file.
 *
 * This function uses the pugixml library to create an XML document and populate it with
 * the control point positions and homography matrix. The control point positions are stored in a 2D array
 * and the homography matrix is stored in a cv::Mat object. Both are saved under their respective
 * XML nodes.
 *
 * @note The XML file is saved to the path specified by the global variable 'configPath'.
 *
 * Example XML structure:
 * @code
 * <config>
 *   <cpPositions>
 *     <Row>
 *       <Cell>value</Cell>
 *       ...
 *     </Row>
 *     ...
 *   </cpPositions>
 *   <H>
 *     <Row>
 *       <Cell>value</Cell>
 *       ...
 *     </Row>
 *     ...
 *   </H>
 * </config>
 * @endcode
 *
 * @return void
 */
void saveCoordinatesXML()
{
    // Create an XML document object
    pugi::xml_document doc;

    // Create the root element "config"
    pugi::xml_node root = doc.append_child("config");

    // Create a child node for storing control point positions
    pugi::xml_node arrayNode = root.append_child("cpPositions");

    // Iterate over the rows of the 2D array 'cpPositions'
    for (const auto &row : cpPositions)
    {
        // Create a row element under "cpPositions"
        pugi::xml_node rowNode = arrayNode.append_child("Row");

        // Iterate over the elements in the row
        for (const auto &value : row)
        {
            // Create a cell element under the row
            pugi::xml_node cellNode = rowNode.append_child("Cell");
            cellNode.append_child(pugi::node_pcdata).set_value(std::to_string(value).c_str());
        }
    }

    // Create a 2D array to store the homography matrix
    float array2[3][3];

    // Copy data from cv::Mat 'H' to the 2D array 'array2'
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            array2[i][j] = H.at<float>(i, j);
        }
    }

    // Create a child node for storing the homography matrix
    pugi::xml_node arrayNode2 = root.append_child("H");

    // Iterate over the rows of the 2D array 'array2'
    for (const auto &row : array2)
    {
        // Create a row element under "H"
        pugi::xml_node rowNode = arrayNode2.append_child("Row");

        // Iterate over the elements in the row
        for (const auto &value : row)
        {
            // Create a cell element under the row
            pugi::xml_node cellNode = rowNode.append_child("Cell");
            cellNode.append_child(pugi::node_pcdata).set_value(std::to_string(value).c_str());
        }
    }

    // Save the XML document to a file specified by 'configPath'
    ROS_INFO("Saving config data to: %s", configPath.c_str());
    if (doc.save_file(configPath.c_str()))
    {
        ROS_INFO("XML file saved successfully.");
    }
    else
    {
        ROS_ERROR("Failed to save XML file.");
    }
}

/**
 * @brief  Entry point for the projection_calibration_node ROS node.
 *
 * This program initializes ROS, DevIL, and GLFW, and then enters a main loop
 * to handle image projection and calibration tasks.
 *
 * @param  argc  Number of command-line arguments.
 * @param  argv  Array of command-line arguments.
 *
 * @return 0 on successful execution, -1 on failure.
 */
int main(int argc, char **argv)
{
    // _______________ SETUP _______________

    // ROS Initialization
    ros::init(argc, argv, "projection_calibration_node", ros::init_options::AnonymousName);
    ros::NodeHandle n;
    ros::NodeHandle nh("~");
    ROS_INFO("Running: main()");

    // TODO: Use rosparam for these settings
    // nh.param<std::string>("configPath", tempPath, "");
    // nh.param<std::string>("windowName", tempName, "");

    // Log paths for debugging
    ROS_INFO("Package Path: %s", packagePath.c_str());
    ROS_INFO("Image Path: %s", imgPath.c_str());
    ROS_INFO("Config XML Path: %s", configPath.c_str());
    ROS_INFO("Display: XYLim=[%0.2f,%0.2f] Width=%d Height=%d AR=%0.2f", xy_lim, xy_lim, winWidth, winHeight, winAspectRatio);
    ROS_INFO("Wall (Norm): Width=%0.2f Space=%0.2f", wallWidth, wallSpace);
    ROS_INFO("Wall (Pxl): Width=%d Space=%d", (int)(wallWidth * (float)winWidth), (int)(wallSpace * (float)winWidth));

    // Initialize DevIL library
    ilInit();

    // Load images
    for (const std::string &img_path : imagePaths)
    {
        ILuint img_id;
        ilGenImages(1, &img_id);
        ilBindImage(img_id);

        ILboolean success = ilLoadImage(img_path.c_str());
        if (success == IL_TRUE)
        {
            imgTestIDs.push_back(img_id);
            ROS_INFO("Loaded image: %s", img_path.c_str());
            ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
        }
        else
        {
            ILenum error = ilGetError();
            ilDeleteImages(1, &img_id);
            ROS_ERROR("DevIL: Failed to load image: Error[%s] File[%s]", iluErrorString(error), img_path.c_str());
        }
    }

    // TODO: Check necessity of these lines
    textureImgWidth = ilGetInteger(IL_IMAGE_WIDTH);
    textureImgHeight = ilGetInteger(IL_IMAGE_HEIGHT);

    // Initialize GLFW
    glfwSetErrorCallback(callbackError);
    if (!glfwInit())
    {
        ROS_ERROR("GLFW Initialization Failed");
        return -1;
    }

    // Create GLFW window
    window = glfwCreateWindow(winWidth, winHeight, windowName.c_str(), NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        ROS_ERROR("GLFW Create Window Failed");
        return -1;
    }

    // Set OpenGL context and callbacks
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glfwSetKeyCallback(window, callbackKeyBinding);
    glfwSetFramebufferSizeCallback(window, callbackFrameBufferSize);

    // Initialize FBO and attach texture to it
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &fboTexture);
    glBindTexture(GL_TEXTURE_2D, fboTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, winWidth, winHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Get the list of available monitors and their count
    monitors = glfwGetMonitors(&monitorCount);
    // TEMP: hardcoding for now
    monitorCount = 2;

    // Set the window to the first monitor
    computeHomography();
    changeWindowMonMode();

    // _______________ MAIN LOOP _______________

    while (!glfwWindowShouldClose(window))
    {
        glEnable(GL_TEXTURE_2D);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw/update wall images
        drawWallsAll();

        // Draw/update control points
        for (int i = 0; i < 4; i++)
        {
            // drawControlPoint(cpPositions[i][0], cpPositions[i][1], cpPositions[i][2], cpPositions[i][3]);
            drawControlPointCircle(cpPositions[i][0], cpPositions[i][1], cpPositions[i][2]);
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Exit condition
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwWindowShouldClose(window))
            break;
    }

    // _______________ CLEANUP _______________

    // Destroy GLFW window and DevIL images
    glfwDestroyWindow(window);
    for (ILuint imageID : imgTestIDs)
    {
        ilDeleteImages(1, &imageID);
    }

    // Shutdown DevIL
    ilShutDown();

    // Terminate GLFW
    glfwTerminate();

    return 0;
}


// Function to load number textures
void loadNumberTextures() {
    // Initialize DevIL once at the beginning of your program
    ilInit();

    // Load each number image and convert it into an OpenGL texture
    // Assume number0.jpg, number1.jpg, ..., number9.jpg
    for (int i = 0; i <= 9; ++i) {
        ILuint imageID;
        ilGenImages(1, &imageID);
        ilBindImage(imageID);

        std::string filename = "number" + std::to_string(i) + ".jpg";
        if (ilLoadImage(filename.c_str())) {
            // Convert to OpenGL texture
            glGenTextures(1, &numberTextures[i]);
            glBindTexture(GL_TEXTURE_2D, numberTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ilGetInteger(IL_IMAGE_WIDTH),
                         ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGB, GL_UNSIGNED_BYTE,
                         ilGetData());
            // ... set texture parameters ...
        }

        ilDeleteImages(1, &imageID);
    }
}


// Function to draw wall with number overlay
void drawWallWithNumber(int number) {
    // ... Your existing drawWall() code ...

    // Draw the number overlay
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, imgMonNumIDs[number]);

    glBegin(GL_QUADS);
    // ... Set texture coordinates and vertex positions ...
    glEnd();

    glDisable(GL_TEXTURE_2D);
}