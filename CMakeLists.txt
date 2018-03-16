CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT (monstrominos)
INCLUDE (FindPkgConfig)

OPTION (WANT_DEBUG "Build the project using debugging code" OFF)
OPTION (WANT_COLORS "Build the project with colors enabled" OFF)
OPTION (WANT_OPENGL "Build the project with OpenGL enabled" OFF)

SET (BASE_DIRECTORY .)
SET (SOURCE_DIR ${BASE_DIRECTORY}/src)
SET (BASIC_SOURCES ${SOURCE_DIR}/monstro-tlogic.c ${SOURCE_DIR}/monstro-tcore.c)
SET (CMAKE_C_FLAGS "-std=gnu99 -fgnu89-inline")
PKG_CHECK_MODULES (ALLEGRO5 allegro-5 allegro_image-5 allegro_font-5 allegro_primitives-5 allegro_color-5 allegro_ttf-5)

IF (WANT_COLORS)
        ADD_DEFINITIONS (-DMONSTRO_TWANT_COLORS)
        SET (BASIC_SOURCES ${BASIC_SOURCES} ${SOURCE_DIR}/monstro-tcolor.c)
ENDIF (WANT_COLORS)

IF (WANT_OPENGL)
	ADD_DEFINITIONS(-DMONSTRO_TWANT_OPENGL)
ENDIF (WANT_OPENGL)

ADD_LIBRARY(BASIC OBJECT ${BASIC_SOURCES})
INCLUDE_DIRECTORIES (${ALLEGRO5_INCLUDE_DIRS} ${BASE_DIRECTORY}/include)
LINK_DIRECTORIES (${ALLEGRO5_LIBRARY_DIRS})

ADD_EXECUTABLE (main ${SOURCE_DIR}/monstro-tallegro5.c $<TARGET_OBJECTS:BASIC>)
TARGET_LINK_LIBRARIES(main ${ALLEGRO5_LIBRARIES} GL GLU)

ADD_EXECUTABLE (ncurses-main ${SOURCE_DIR}/monstro-tncurses.c $<TARGET_OBJECTS:BASIC>)
TARGET_LINK_LIBRARIES(ncurses-main ncurses)
