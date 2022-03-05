
function(ConfigureOutputDirectory Source Dest)
    make_directory(${Dest})

    file(GLOB Files RELATIVE ${Source} ${Source}/*)
    foreach(File ${Files})
        set(Path ${Source}/${File})
        if(NOT IS_DIRECTORY ${Path})
            configure_file(${Path} ${Dest}/${File} COPYONLY)
        endif(NOT IS_DIRECTORY ${Path})
    endforeach(File)
endfunction(ConfigureOutputDirectory)
