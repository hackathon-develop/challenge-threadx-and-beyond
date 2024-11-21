{ ... }:
{
  # Used to find the project root
  projectRootFile = "flake.nix";

  settings.global.excludes = [
    "MXChip/AZ3166/app/startup/*"
    "MXChip/AZ3166/app/stm32cubef4/*"
    "MXChip/AZ3166/lib/*"
    "shared/lib/*"
    "shared/src/*"
  ];

  programs.clang-format.enable = true;
  settings.formatter.clang-format.options = [ "-style=file:${./.clang-format}" "-i" ];

  programs.nixpkgs-fmt.enable = true;

  programs.prettier = {
    enable = true;
    includes = [
      "*.css"
      "*.html"
      "*.js"
      "*.json"
      "*.json5"
      "*.md"
      "*.mdx"
      "*.yaml"
      "*.yml"
    ];
  };

}
