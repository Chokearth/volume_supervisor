{ pkgs, ... }:

{
  packages = with pkgs; [ 
    git
    nodePackages.pnpm
    alsa-utils
  ];

  enterShell = ''
    pnpm i
    clear
    pfetch
    pulseaudio
  '';

  languages.javascript.enable = true;
}
