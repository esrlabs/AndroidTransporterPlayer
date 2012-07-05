cxx_configuration do
  exe 'AndroidTransporterPlayer',
    :sources => FileList['*.cpp'],
    :includes => ['.'],
    :dependencies => ['android-os'] + bin_libs('ilclient', 'GLESv2', 'EGL', 'openmaxil', 'bcm_host')
end
