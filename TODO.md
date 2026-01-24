# TODO

- document everything
- node renderer/editor
- reorganise the drawing/canvas system to be more abstract - something akin to draw commands!
- custom terminal renderer window with custom font to bypass stupid terminal support
- reorganise window manager system (separate this from rasterisation)
  - rasteriser - collects draw commands, executes them, turns the rendered stuff into displayable format for a particular output
  - context - canvas, provides drawing utils
  - window manager
- palette system, saved on the context? with a stack so extra styles can be pushed, eliminate per-command colour info? (or give alternate versions which assume the palette)
