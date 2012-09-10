cxx_configuration do
  bin_libs('GLESv2', 'EGL', 'openmaxil', 'vchiq_arm')
  bin_lib('ilclient',
          {
            :includes => ["#{RASP_ROOTFS}/opt/vc/src/hello_pi/libs/ilclient"],
            :lib_path => "#{RASP_ROOTFS}/opt/vc/src/hello_pi/libs/ilclient"
          })
  bin_lib('bcm_host',
          {
            :includes => ["#{RASP_ROOTFS}/opt/vc/include"],
            :lib_path => "#{RASP_ROOTFS}/opt/vc/lib"
          })
  bin_lib('vcos',
          {
            :includes => ["#{RASP_ROOTFS}/opt/vc/include/interface/vcos/pthreads"],
            :lib_path => "#{RASP_ROOTFS}/opt/vc/lib"
          })

  exe 'AndroidTransporterPlayer',
    :sources => FileList['*.cpp','rtcp/**/*.cpp'],
    :includes => ['.'],
    :dependencies => ['mindroid', 'fdk-aac', 'ilclient', 'GLESv2', 'EGL', 'openmaxil', 'bcm_host', 'vcos', 'vchiq_arm']
end
