# Z view: implementation plan

A plan for bringing back the dedicated Z view that GtkRadiant 1.3 had: a narrow pane that shows
the vertical extent of the brushes under one point of the map, with the camera height marked on
it. It is built into the editor core, not as a plugin, because the plugin API cannot dock a
widget into the main window.

File and line references are to commit `b1994f8`.

## Background

- The Z window was removed in the GtkRadiant 1.5 rewrite. Only stubs remain here: the `ZWnd`
  forward declaration, `m_pZWnd` and `GetZWnd()` in `radiant/mainframe.h`. The class is defined
  nowhere.
- The old `z.cpp` cannot be reused. It walked global brush lists that no longer exist. It is a
  behavioural reference only.
- The original sampled brushes at a movable "Z checker" marker drawn in the XY view. This is
  from memory and has not been checked against the 1.3 source. The plan calls it the probe
  point.

## Design choices for both phases

- **New files**: `radiant/zwindow.h` and `radiant/zwindow.cpp`, plus `radiant/zwindow.o` in the
  object list near `Makefile:967`.
- **Docking**: wrap the XY view in a new horizontal `QSplitter` that holds
  `[Z view | XY view]`, and put that splitter where the XY widget goes today. Do not add the Z
  view to `m_hSplit` directly: `MaximizeView` (`radiant/mainframe.cpp:1550`),
  `RestoreGuiState` and the built-in group dialog all assume `m_hSplit` has exactly two
  children.
- **Brush extents**: clip the vertical line through the probe point against each brush's face
  planes. This gives the exact top and bottom Z of a convex brush without a ray-versus-winding
  test.
- **Drawing**: immediate-mode GL through `gl()`, the same way `XYWnd` draws its grid.
- **Colours**: reuse `g_xywindow_globals` (grid, text, selected-brush and camera colours), so
  existing viewport themes apply and no colour dialog entries are needed.

## Phase 1: display-only pane with navigation

### 1. `ZWnd` and its GL widget

- Model a `ZGLWidget : QOpenGLWidget` on `XYGLWidget` (`radiant/xywindow.cpp:436`):
  `glwidget_context_created` and `glwidget_context_destroyed`, device-pixel-ratio scaling,
  `DeferredDraw`.
- Draw straight to the widget. Skip the FBO and overlay pass that `XYWnd` uses; there is no
  rubber-band rectangle to overlay.
- State: vertical origin, scale, width and height, and the probe point.

### 2. Layout and visibility

- Regular and regular-left layouts (`radiant/mainframe.cpp:1717`): create the nested splitter
  and register it with `g_guiSettings.addSplitter( ..., "MainFrame/m_zSplit", ... )` in
  `RestoreGuiState`, so the width persists.
- Floating layout (`:1752`): its own window registered as `"floating/z"`, following the XZ and
  YZ window blocks.
- 4-way split layout (`:1829`): left out, since XZ and YZ are already visible there.
- A `ToggleShown g_z_shown` with a `ToggleZView` toggle, a `ZVIS` preference (as `XYVIS` at
  `radiant/xywindow.cpp:2306`), and a "Z View" item in the View menu next to the entries at
  `radiant/mainframe.cpp:937`.
- Delete `m_pZWnd` in `MainFrame::Shutdown` alongside the XY windows.

### 3. Drawing

- Background and horizontal grid lines at the current grid size, with major and minor steps
  and Z coordinate labels, adapted from `XYWnd::XY_DrawGrid` (`radiant/xywindow.cpp:1279`).
- Brush columns: walk with `Scene_forEachVisibleBrush`, reject brushes whose `worldAABB()` does
  not contain the probe point, then do the plane clip. Fill each column with the top face's
  texture colour (`qtexture_t::color`) and outline it in black. Outline selected brushes in
  `color_selbrushes`.
- Camera icon at the camera's Z, from `Camera_getOrigin`.
- The probe point follows the camera's X and Y in this phase.

### 4. Navigation

- Wheel to zoom. The XY panning button combination (`Move_buttons()`) scrolls vertically.
- The XY "move camera" combination (`MoveCamera_buttons()`, see `radiant/xywindow.cpp:969`)
  sets the camera height through `Camera_setOrigin`.

### 5. Redraw triggers

- `AddSceneChangeCallback`, `AddCameraMovedCallback`, the selection-changed callback,
  `AddGridChangeCallback` and the map-valid callback.
- Add the Z view to `XY_UpdateAllWindows` (`radiant/mainframe.cpp:848`).

### Done when

The pane docks left of the XY view in both regular layouts, floats in the floating layout,
keeps its width and visibility across a restart, works with F12 maximise, and follows camera
movement, selection and edits live.

## Phase 2: editing

### 1. Probe marker

- Draw the probe as a small icon in the XY view and let a click there move it. Keep "follow
  camera" as an option and as the default.
- This needs a free mouse combination in the XY view, where bindings are crowded.

### 2. Hit testing and selection

- Map a click to the brush column whose Z interval contains it. Where columns overlap, prefer
  the smallest interval.
- Select with the same modifier clicks as the XY view, through each instance's `Selectable`.

### 3. Vertical move

- Dragging the body of a selected column moves the whole selection in Z, snapped to the grid.
- Use the selection system's own path: `translateSelected` (`radiant/selection.cpp:1698`)
  during the drag and `freezeTransforms` on release, inside one `UndoableCommand`, so a drag is
  a single undo step.

### 4. Top and bottom face drag

- Dragging near the top or bottom edge of a column moves only that face plane, as the original
  did.
- This is the riskiest item. The existing face-drag code (`selectPlanes`,
  `radiant/brush.h:3824`) is driven by a `SelectionTest` built from a view. The Z view must
  either build a synthetic one, or translate the face plane directly and re-validate the brush.

### 5. Finishing

- Escape cancels a drag, the cursor changes over draggable edges, and a size readout shows
  while dragging.
- Add an entry to `docs/changelog-custom.txt`.
- Rebuild and commit the binaries under `install/`, as `CLAUDE.md` requires.

### Done when

Select, move and face resize each work with undo and redo, respect grid snap, and leave brushes
valid when a face is dragged past its opposite.

## Risks

- **Transformed brushes**: brushes inside entities with a non-identity `localToWorld` need
  their planes transformed before clipping. This is easy to miss, because worldspawn brushes
  work without it.
- **Large maps**: every redraw walks all visible brushes. The bounds reject makes this cheap,
  but check it on a map with 20,000 or more brushes, since camera movement triggers redraws.
- **Patches and point entities**: not shown in either phase, matching the original. Bounds-only
  patch columns would be a small addition.

## Open decisions

- **Default visibility**: hidden by default with the toggle, so the layout is unchanged until
  the view is turned on, or shown by default.
- **Probe marker binding**: which XY mouse combination moves the marker in phase 2.
