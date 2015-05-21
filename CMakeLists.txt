cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(Ensitsp)
enable_testing()
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -O3 -std=gnu++11")

#########
# Gestion des logins
#########
# Vous devez editer la ligne suivante pour y mettre vos logins
#########
set(LOGINS perrettc dangeloan)
list(SORT LOGINS)

if (LOGINS MATCHES "login[123]")
  message(FATAL_ERROR "** ERREUR **: Vous devez modifier CMakeLists.txt pour y mettre vos logins")
endif()



#####
# Détection de la bibliothèque de threads et de la bibliothèque de math
#####
find_package(Threads)
find_library(MISC_LIBRARY NAMES m libm rt librt)

##### ENSIMAG-SPECIFIC
# Utilisation d'un éventuel g++ et gcc dans /opt/gnat/bin
# (CentOS 6 est en gcc 4.4, sans support de C++-11)
#####
if(EXISTS /opt/gnat/bin)
  set(CMAKE_C_COMPILER /opt/gnat/bin/gcc)
  set(CMAKE_CXX_COMPILER /opt/gnat/bin/g++)
endif(EXISTS /opt/gnat/bin)

###
# Détection d'un solveur linéaire disponible (COINOR-CBC,
# COINOR-SYMPHONY ou GLPK-GLPSOL)
###
# Ordre de préférence: glpsol, symphony, cbc
# Raison: le programme linéaire est stupide, glpk est le
# solveur le plus rapide, parce que le plus lèger à démarrer
# 
# Si le programme linéaire devient plus intelligent, il faudra revoir
# les choix

find_program(CBC cbc)
find_program(SYMPHONY symphony)
find_program(GLPSOL glpsol)

if (GLPSOL)
  set(SOLVEUR 2)
elseif(SYMPHONY)
  set(SOLVEUR 1)
elseif(CBC)
  set(SOLVEUR 0)
else()
  set(SOLVEUR 3)
  message(STATUS "** ERREUR **: aucun solveur linéaire trouvé, votre calcul sera plus lent pour les instance >= 20 villes")
endif()

configure_file (
  src/lpsolveurs.h.in
  ${CMAKE_SOURCE_DIR}/src/lpsolveurs.h
  )

##
# Si vous utilisez d'autres fichiers .c, en plus de ceux listés,
# il faut les ajouter ici
##
add_executable(ensitsp src/tsp-genmap.c  src/tsp-job.c  src/tsp-main-para.c  src/tsp-print.c  src/tsp-tsp.c src/tsp-lp.c src/tsp-hkbound.cpp)
target_link_libraries(ensitsp ${CMAKE_THREAD_LIBS_INIT} m rt)

##
# Programme de test
##
add_test(Tests ../tests/validite.pl)
add_custom_target(check ../tests/validite.pl)

#add_test(MesuresDePerf ../mesures/mesures.pl)


##
# Construction de l'archive
##
string(REPLACE ";" "-" LOGINS_SANS_POINTVIRGULE "${LOGINS}")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH ${LOGINS_SANS_POINTVIRGULE})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  "~$"
  "\\\\.o$"
  "^${PROJECT_SOURCE_DIR}/build/"
  "^${PROJECT_SOURCE_DIR}/.git/"
)
include(CPack)
