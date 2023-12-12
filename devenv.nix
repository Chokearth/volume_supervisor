{ pkgs, ... }:

{
  packages = with pkgs; [ 
    git
    nodePackages.pnpm
    alsa-utils
    pulseaudio
  ];

  enterShell = ''
    pnpm i
    clear
    pfetch
  '';

  languages.javascript.enable = true;
}
