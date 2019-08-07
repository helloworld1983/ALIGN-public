This directory contains a set of docker services that run the end-to-end ALIGN flow.

Either you can invoke the flow from the shell using a working
directory, or you can use a make-docker-service to invoke the flow
using a docker volume (the latter works in CircleCI and in WSL)>

To invoke using a working directory:

		% export ALIGN_HOME=<top of ALIGN source area>
		% export ALIGN_WORKING_DIR=<directory path for output>
		% cd $ALIGN_WORKING_DIR
		% ln -sf $ALIGN_HOME/compose/Makefile .
		% make DESIGN=<design>

> This will bring up the set of docker services that can run the steps
> in the flow, then invoke a series of commands in each container to
> execute the flow.

To invoke using a Docker volume:

		% docker volume create compose_dataVolume
		% export $ALIGN_HOME=<top of ALIGN source area>
		% cd $ALIGN_HOME/compose
		% docker-compose up -d make-docker-service
		% docker-compose exec make-docker-service make DESIGN=<design>

> This will first bring up a make-docker-service which contains the
> main Makefile and docker-compose configuration.  The exec will then
> use the make-docker-service to also bring up the rest of the
> services.  After that, the make will run the flow for the given design.

# Useful docker-compose commands

You must be in the directory where the service configuration file
(usually docker-compose.yml) resides.  If the directory name does not
match, then the services can only be found by using the -p <project>
where project is the original directory name or project name used to
bring up the services.

  - docker-compose up -d:  bring up all services (build images if needed)
  
  - docker-compose down:  shut down all services and remove containers
    - --rmi: remove images too
	
  - docker-compose up -d <service>:  bring up a specific service
  
  - docker-compose up -d --build <service>:  build and bring up a service
  
  - docker-compose exec <service> <command>:  execute a command on a running container
