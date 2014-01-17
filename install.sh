#!/bin/bash

if [ ! -e "./build" ]; then
  mkdir build
fi

if [ ! -e "./run" ]; then
  mkdir run
fi

make

# nom du projet utiliser pour ftok()
project="anneau"

# start_anneau.sh
echo "#!/bin/bash" > start_anneau.sh
echo "./run/anneau $project 500" >> start_anneau.sh

echo "#!/bin/bash" > start_anneau_100ms.sh
echo "./run/anneau $project 100" >> start_anneau_100ms.sh

# start_server
echo "#!/bin/bash" > start_server.sh
echo "./run/server $project" >> start_server.sh

# start_robot_1.sh
echo "#!/bin/bash" > start_robot_1.sh
echo "./run/robot $project 1 125 1234 1234" >> start_robot_1.sh

# start_robot_2.sh
echo "#!/bin/bash" > start_robot_2.sh
echo "./run/robot $project 2 21 12 1234" >> start_robot_2.sh

# start_robot_3.sh
echo "#!/bin/bash" > start_robot_3.sh
echo "./run/robot $project 3 346 13 1234" >> start_robot_3.sh

# start_robot_4.sh
echo "#!/bin/bash" > start_robot_4.sh
echo "./run/robot $project 4 43 24 1234" >> start_robot_4.sh

# start_robot_5.sh
echo "#!/bin/bash" > start_robot_5.sh
echo "./run/robot $project 5 5 13 0" >> start_robot_5.sh

# start_robot_6.sh
echo "#!/bin/bash" > start_robot_6.sh
echo "./run/robot $project 6 6 24 0" >> start_robot_6.sh

chmod +x *.sh