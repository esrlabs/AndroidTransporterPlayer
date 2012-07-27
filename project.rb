cxx_configuration do
  exe 'AndroidTransporterPlayer',
    :sources => FileList['*.cpp','rtcp/**/*.cpp'],
    :includes => ['.'],
    :dependencies => ['mindroid'] + bin_libs('ilclient', 'GLESv2', 'EGL', 'openmaxil', 'bcm_host', 'vcos', 'vchiq_arm')
end
