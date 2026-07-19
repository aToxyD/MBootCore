function(install_mbootcore_targets)
    install(TARGETS mbootcore
        EXPORT MBootCoreTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mbootcore
    )
    install(EXPORT MBootCoreTargets
        NAMESPACE MBootCore::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MBootCore
    )
endfunction()

function(install_mbootcore_headers)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/lib/include/mbootcore/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mbootcore
        FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "*.h"
    )
endfunction()

function(install_mbootsdk_headers)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/sdk/include/sdk/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sdk
        FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "*.h"
    )
endfunction()

function(install_mbootcore_examples)
endfunction()

function(install_mbootcore_docs)
endfunction()
