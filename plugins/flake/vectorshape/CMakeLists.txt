project(vectorshape)

set ( VectorShape_SRCS
    VectorShapePlugin.cpp
    VectorShape.cpp
    VectorShapeConfigWidget.cpp
    VectorShapeFactory.cpp
    VectorTool.cpp
    VectorToolFactory.cpp
    ChangeVectorDataCommand.cpp
)

add_library(krita_shape_vector MODULE ${VectorShape_SRCS})

target_link_libraries(krita_shape_vector kritaflake kritawidgets kritavectorimage KF5::I18n Qt5::Svg)

install(TARGETS krita_shape_vector DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

