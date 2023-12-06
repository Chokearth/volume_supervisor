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
  '';

  languages.javascript.enable = true;
}
