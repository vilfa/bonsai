# -*- mode: ruby -*-
# vi: set ft=ruby :

module LocalCommand
  class Config < Vagrant.plugin("2", :config)
    attr_accessor :command
  end
  
  class Plugin < Vagrant.plugin("2")
    name "local_shell"
    
    config(:local_shell, :provisioner) do
      Config
    end
    
    provisioner(:local_shell) do
      Provisioner
    end
  end
  
  class Provisioner < Vagrant.plugin("2", :provisioner)
    def provision
      result = system "#{config.command}"
    end
  end
end

Vagrant.configure("2") do |config|
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.
  
  # Every Vagrant development devenvment requires a box. You can search for
  # boxes at https://vagrantcloud.com/search.
  config.vm.define "devenv"
  config.vm.box = "generic/arch"
  config.vm.hostname = "bonsai-devenv"
  
  # Configure virtualbox for 3d accelerated graphical session.
  config.vm.provider :virtualbox do |m|
    m.gui = true
    m.memory = 2048
    m.cpus = 4
    m.customize ["modifyvm", :id, "--vram", "256"]
    m.customize ["modifyvm", :id, "--accelerate3d", "on"]
  end

  # Configure libvirt for 3d accelerated graphical session.
  config.vm.provider :libvirt do |m|
    m.graphics_type = "spice"
    m.video_accel3d = true
    m.video_type = "virtio"
    m.graphics_ip = nil
    m.graphics_port = nil
    m.uuid = "60fa5b18-e4d6-47a4-af7a-023d3d34bfaa"
    m.uri = "qemu:///system"
  end
  
  # Share working directory with devenv.
  config.vm.synced_folder ".", "/vagrant_data"
  
  # Various provisioners for working with the devenv.
  config.vm.provision "connect", type: "local_shell", run: "always" do |p|
    p.command = "virt-viewer -ac qemu:///system --uuid 60fa5b18-e4d6-47a4-af7a-023d3d34bfaa &"
  end
  
  config.vm.provision "prepare_box", type: "shell", run: "once" do |p|
    p.path = "devenv/prepare_box.sh"
    p.privileged = true
  end
  
  config.vm.provision "install_compile_deps", type: "shell", run: "once" do |p|
    p.path = "devenv/install_compile_deps.sh"
    p.privileged = true
  end
  
  config.vm.provision "install_runtime_deps", type: "shell", run: "once" do |p|
    p.path = "devenv/install_runtime_deps.sh"
    p.privileged = true
  end
  
  config.vm.provision "prepare_git", type: "shell", run: "never" do |p|
    p.path = "devenv/project_prepare_git.sh"
    p.privileged = false
  end
  
  config.vm.provision "prepare_local", type: "shell", run: "never" do |p|
    p.path = "devenv/project_prepare_local.sh"
    p.privileged = false
  end
  
  config.vm.provision "build", type: "shell", run: "never" do |p|
    p.path = "devenv/project_build.sh"
    p.privileged = false
  end
  
  config.vm.provision "update", type: "shell", run: "never" do |p|
    p.path = "devenv/project_update.sh"
    p.privileged = false
  end
  
  config.vm.provision "run", type: "shell", run: "never" do |p|
    p.path = "devenv/project_run.sh"
    p.privileged = false
  end
  
  config.vm.provision "kill", type: "shell", run: "never" do |p|
    p.path = "devenv/project_kill.sh"
    p.privileged = false
  end
  
  config.vm.provision "clean", type: "shell", run: "never" do |p|
    p.path = "devenv/project_clean.sh"
    p.privileged = false
  end
end
