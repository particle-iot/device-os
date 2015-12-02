# build the zip file for uploading to AWS
mkdir Particle_Windows_Serial_Drivers
cp *.cat Particle_Windows_Serial_Drivers
cp *.inf Particle_Windows_Serial_Drivers
cp readme.md Particle_Windows_Serial_Drivers/README.txt
zip Spark.zip Particle_Windows_Serial_Drivers/*
rm -rf Particle_Windows_Serial_Drivers

