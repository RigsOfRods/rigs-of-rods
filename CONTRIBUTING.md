# Contributing

Here's a quick guide on how to proceed if you want to contribute. The first section will deal with reporting bugs and the second section summarizes the steps for developers wanting to contribute code.

## Reporting bugs

* Before you open a new issue please browse the existing issues to see if there already is an open ticket for your issue to prevent duplicates
* Include all necessary information, the more information you give the less time consuming it will be for the devs to work on this issue
* Put logs and console output into code tags: \`\`\` \<your output here\> \`\`\`
* Use this template if possible:


```
### Steps to reproduce
1.
2.
3.

### Expected behaviour
Tell us what should happen

### Actual behaviour
Tell us what happens instead

### System configuration
OS: e.g. Windows 7 64bit
Renderer used: e.g. DirectX 9 or OpenGL (this does actually affect physics!)

optional:
GPU and video driver version if your issue is related to graphics

### Additional information, logs and screenshots (optional)
e.g. RoR.log (in code tags!)
e.g. used vehicle + link to download
e.g. screenshot of graphical glitch
```


  <br>
  <br>

## Contribute code


### Setup
* Fork, then clone the repository
* Set up your development environment, help may be found [here][compile]
* Make sure Rigs of Rods runs properly before doing any changes

### Applying changes
* Make your change
    * Follow our [style guide][style]
    * Write a good commit message, respecting our [prefixes][commit]
* Make sure it is compatbile with all major compilers and platforms (Windows, Linux, OS X)
    * We don't expect you to test on all platforms but you have to avoid compiler/platform specific code
* Test your change, watch out for regression

### Merge into upstream
* Push to your online repository and submit a pull request
* At this point you're waiting on us. We may suggest changes, improvements or alternatives

#### Additional links:
* [Developer Wiki Portal][devwiki]
* [Doxygen documentation][doxy]

[compile]: https://github.com/RigsOfRods/rigs-of-rods/wiki
[style]: https://github.com/RigsOfRods/rigs-of-rods/wiki/Coding-style
[commit]: https://github.com/RigsOfRods/rigs-of-rods/wiki/Commit-style
[devwiki]: http://www.developer.rigsofrods.org/
[doxy]: http://www.developer.rigsofrods.org/doxygen/
