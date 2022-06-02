#To get ddio bus: lspci -vvv | grep Mell
gcc change-ddio.c -o change-ddio -lpci
sudo ./change-ddio