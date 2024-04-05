% MATLAB code to draw a square with specified vertices and
% adjust the physical size of the figure and axes to match the axis limits

close all;

im_wd = 300;
im_ht = 540;

ax_wd = 1000;
ax_ht = 1000;

% Create the figure
fig = figure;

% Calculate the ratio of axis limits and set the figure size accordingly
fig.Units = 'pixels';
fig.Position = [0, 0, ax_wd+100, ax_ht+100];
movegui("east");

% Create the axes and set their Position and Units
ax = axes;
axis();
set(ax, ...
    'Units', 'pixels', ...
    'Position', [50, 50, ax_wd, ax_ht], ...
    'XLim', [0, ax_wd], ...
    'YLim', [0, ax_ht], ...
    'YDir', 'reverse');

% Define the coordinates of the square vertices
srcPoints = [
    0, 0;
    im_wd, 0;
    im_wd, im_ht;
    0, im_ht
    ];
src = srcPoints;

% Define the coordinates of the square vertices
dstPoints = [
    src(1,1) + 025.0, src(1,2) + 000.0;       % V1 (top left)
    src(2,1) + 025.0, src(2,2) - 025.0;       % V2 (top right)
    src(3,1) - 050.0, src(3,2) - 075.0;       % V3 (bottom right)
    src(4,1) + 000.0, src(3,2) + 000.0        % V4 (bottom left)
    ];

% define offset to center dst
offset_x = (ax_wd-im_wd)/2;
offset_y = (ax_ht-im_ht)/2;

%offset_x = 10;
%offset_y = 10;

% Add offset
for i = 1:4
    dstPoints(i,1) = dstPoints(i,1) + offset_x;
    dstPoints(i,2) = dstPoints(i,2) + offset_y;
end

% Define the order to connect vertices
order = [
    1;  % V0 (top left)
    2;  % V1 (top right)
    3;  % V2 (bottom right)
    4;  % V3 (bottom left)
    1
    ];

% Plot quads
plotQuad(srcPoints, order, 'b');
plotQuad(dstPoints, order, 'r');

% Format variable string
fprintf('\n\n\n');
formatForCpp(srcPoints, 'srcPoints');
formatForCpp(dstPoints, 'dstPoints');
fprintf('\n\n\n');

function plotQuad(quad_vertices, order, col)

% Plot the square
hold on;
for i = 1:length(order) - 1
    start_idx = order(i);
    end_idx = order(i + 1);
    x_start = quad_vertices(start_idx, 1);
    y_start = quad_vertices(start_idx, 2);
    x_end = quad_vertices(end_idx, 1);
    y_end = quad_vertices(end_idx, 2);
    plot([x_start, x_end], [y_start, y_end], col);
end

% Label vertices
for i = 1:size(quad_vertices, 1)
    x = quad_vertices(i, 1);
    y = quad_vertices(i, 2);
    val_str = [sprintf('V%d', i-1), '_{', sprintf('(%+d,%+d)', x, y), '}'];
    text(x, quad_vertices(i, 2), ...
        val_str, ...
        'Interpreter', 'tex', ...
        "FontSize", 14, ...
        "FontWeight", "bold", ...
        'Color', col, ...
        'VerticalAlignment', 'Top', ...
        'HorizontalAlignment', 'Left');
end

end

function formatForCpp(quad_vertices, var_str)
% Initialize the string with the opening of the C++ vector declaration
fprintf('\nstd::vector<cv::Point2f> %s = {\n    ', var_str);

% Loop through the array to format each entry
for i = 1:size(quad_vertices, 1)
    x = quad_vertices(i, 1);
    y = quad_vertices(i, 2);

    % Add a comma and newline if it's not the first entry
    if i > 1
        fprintf(',\n    ');
    end

    % Print the current point
    fprintf('cv::Point2f(%d, %d)', x, y);
end

% Close the curly brace and add a newline
fprintf('\n};\n');
end





