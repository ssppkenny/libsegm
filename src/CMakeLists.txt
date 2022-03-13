set(SEGM_SRC
	"common.cpp"
	"Enclosure.cpp"
	"ImageNode.cpp"
	"LineSpacing.cpp"
	"PageSegmenter.cpp"
	"Reflow.cpp"
	"Xycut.cpp"
	)

add_library(segm STATIC ${SEGM_SRC})
target_include_directories(segm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
