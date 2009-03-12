JaikuUiKit provides UI components and tools for UI creation. 
JaikuUiKit is generic, and doesn't model Jaiku business logic.

JaikuUiKit shouldn't have dependencies anywhere else than ContextCommon and 
from there only use "context style C++" components, nothing application or 
business logic specific. However, in first phase, actual 
layout values are still here and there are temporary dependency to ContextUi
because of icon loading code resides still there 

Currently provides:
- layout data handling 
- generic listbox
- generic renderers
