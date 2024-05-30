#!/bin/bash

group_dir_name="$HOME/group_mm_"
ind_dir_name="$HOME/ind_mm_"
log_dir_name="$HOME/logs/"
anim_dir_name="$HOME/anims/"
temp_log_file=$log_dir_name"temp.log"
waf_path="$HOME/workspace/bake/source/ns-3.25/"
manet_path="$HOME/MANET/"

seed=(1 5 7 13 56 43 66 12 87 132)
# seed=(1)

for elem in "${seed[@]}"
do
    group_dir_name_full=$group_dir_name$elem/
    ind_dir_name_full=$ind_dir_name$elem/

    
    if [ -d "$group_dir_name_full" ]; then
        rm -r "$group_dir_name_full"
    fi

    if [ -d "$ind_dir_name_full" ]; then
        rm -r "$ind_dir_name_full"
    fi

    if [ -d "$log_dir_name" ]; then
        rm -r "$log_dir_name"
    fi

    if [ -d "$anim_dir_name" ]; then
        rm -r "$anim_dir_name"
    fi

    if [ -e "$temp_log_file" ]; then
        rm "$temp_log_file"
    fi

    # Creating mobility models

    mkdir $group_dir_name_full
    mkdir $ind_dir_name_full
    mkdir $log_dir_name
    mkdir $anim_dir_name
    touch $temp_log_file
    touch $log_dir_name"olsr.group."$elem".json"
    touch $log_dir_name"olsr.ind."$elem".json"

    lua mmgen.lua rgmm.conf $group_dir_name_full $elem
    lua mmgen.lua rwmm.conf $ind_dir_name_full $elem


    # Running OLSR

    
    cd $waf_path
    olsr_anim_path=$anim_dir_name"olsr.group."$elem".xml"
    ./waf --run "main --rProtocol=OLSR --outDirPath=$group_dir_name_full --animPath=$olsr_anim_path" 2> $temp_log_file
    
    cd $manet_path
    python olsr_log_reader.py $temp_log_file > $log_dir_name"olsr.group."$elem".json";
    

    cd $waf_path
    olsr_anim_path=$anim_dir_name"olsr.ind."$elem".xml"
    ./waf --run "main --rProtocol=OLSR --outDirPath=$ind_dir_name_full --animPath=$olsr_anim_path" 2> $temp_log_file
    
    cd $manet_path
    python olsr_log_reader.py $temp_log_file > $log_dir_name"olsr.ind."$elem".json";


    # Running BATMAN

    # cd $waf_path
    # bat_anim_path=$anim_dir_name"batman.group."$elem".xml"
    # ./waf --run "main --rProtocol=BATMAN --outDirPath=$group_dir_name_full --animPath=$bat_anim_path" 2> $temp_log_file
    
    # cd $manet_path
    # python batman_log_reader.py $temp_log_file > $log_dir_name"batman.group."$elem".json";

    # cd $waf_path
    # bat_anim_path=$anim_dir_name"batman.ind."$elem".xml"
    # ./waf --run "main --rProtocol=BATMAN --outDirPath=$ind_dir_name_full --animPath=$bat_anim_path" 2> $temp_log_file
    
    # cd $manet_path
    # python batman_log_reader.py $temp_log_file > $log_dir_name"batman.ind."$elem".json";

done