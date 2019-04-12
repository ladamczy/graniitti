#!/bin/sh
#
# Run with: source ./tests/harmonic/run.sh

read -p "cmsharmonicrun: Generate events (or only analyze)? [y/n] " -n 1 -r
echo # New line

if [[ $REPLY =~ ^[Yy]$ ]]
then

# Generate
./bin/gr -i ./tests/cmsharmonicrun/SH_2pi_REF_CMS.json -n 100000
./bin/gr -i ./tests/cmsharmonicrun/SH_2pi_CMS.json     -n 100000

fi

# ***********************************************************************
# Fiducial cuts <ETAMIN,ETAMAX,PTMIN,PTMAX>
FIDCUTS=-2.5,2.5,0.1,100.0
# ***********************************************************************

# System kinematic variables binning <bins,min,max>
MBINS=30,0.28,1.75
PBINS=1,0.0,1.75
YBINS=1,-0.9,0.9

# PARAMETERS
LMAX=4
REMOVEODD=true
REMOVENEGATIVE=true
SVDREG=1e-4
L1REG=0 #1e-5
EML=true

# Lorentz frames
for FRAME in HE CS PG SR
do

# Analyze
./bin/fitharmonic -r SH_2pi_REF_CMS -i SH_2pi_CMS -s true -t MC \
-c $FIDCUTS \
-f $FRAME -l $LMAX -o $REMOVEODD -n $REMOVENEGATIVE -a $SVDREG -b $L1REG -e $EML \
-M $MBINS -P $PBINS -Y $YBINS \
-s $FS \
-X 100000

done

# Implement 2D harmonic plots (M,Pt)
# ...
