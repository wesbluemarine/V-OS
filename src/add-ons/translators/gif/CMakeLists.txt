
Application(
	GIFTranslator

	SOURCES
	GIFLoad.cpp	
	GIFSave.cpp
	GIFTranslator.cpp
	GIFView.cpp
	LoadPalette.cpp
	SavePalette.cpp
	SFHash.cpp

	LIBS be translation translatorutils localestub ${GIF_LIBRARIES}

	INCLUDES
	"../shared"
)
