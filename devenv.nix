{ pkgs, ... }:

{
  packages = with pkgs; [ 
    git
    nodePackages.pnpm
  ];

  enterShell = ''
    pnpm i
    clear
    pfetch
  '';

  languages.javascript.enable = true;
}
