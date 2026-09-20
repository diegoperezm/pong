/media/dapm/projects/pong/src/simulation.c: In function ‘sync_paddles_to_physics’:
/media/dapm/projects/pong/src/simulation.c:293:7: error: ‘dt’ undeclared (first use in this function)
  293 |     / dt;
      |       ^~
/media/dapm/projects/pong/src/simulation.c:293:7: note: each undeclared identifier is reported only once for each function it appears in
/media/dapm/projects/pong/src/simulation.c: In function ‘scoring_system’:
/media/dapm/projects/pong/src/simulation.c:603:9: error: too few arguments to function ‘particle_spawn’
  603 |         particle_spawn(
      |         ^~~~~~~~~~~~~~
In file included from /media/dapm/projects/pong/src/simulation.c:3:
/media/dapm/projects/pong/src/entities.h:28:6: note: declared here
   28 | void particle_spawn(
      |      ^~~~~~~~~~~~~~
/media/dapm/projects/pong/src/simulation.c:622:9: warning: implicit declaration of function ‘particles_spawn’; did you mean ‘particle_spawn’? [-Wimplicit-function-declaration]
  622 |         particles_spawn(
      |         ^~~~~~~~~~~~~~~
      |         particle_spawn
gmake[2]: *** [CMakeFiles/pong.dir/build.make:132: CMakeFiles/pong.dir/src/simulation.c.o] Error 1
gmake[1]: *** [CMakeFiles/Makefile2:183: CMakeFiles/pong.dir/all] Error 2
gmake: *** [Makefile:156: all] Error 2
