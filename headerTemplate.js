var myHeaderPrototype = Object.create(HTMLElement.prototype);

myHeaderPrototype.createdCallback = function() {
  let t = document.querySelector('my-header');

  //change the argument here
  //if you move the domain
  //also for this to be correct in your working directory,
  //the containing folder for these files needs to be
  //named the argument passed in here
  let relativePathLevel = getLevelsDeep('epoch-language');

  let relativeHomeLink = "";
  let relativeOverviewLink = "";
  let relativeArticlesLink = "";
  
  //Add ../s
  if(relativePathLevel == 0) {
     relativeHomeLink += "./";
     relativeOverviewLink += "./";
     relativeArticlesLink += "./";
  }
  else {
    for(let i = 0; i < relativePathLevel; ++i)
    {
      relativeHomeLink += "../";
      relativeOverviewLink += "../";
      relativeArticlesLink += "../";
    }
  }
 
  
  relativeHomeLink += "index.html";
  relativeOverviewLink += "project_overview.html";
  relativeArticlesLink += "articles/index.html";

  t.innerHTML = `
  <div class="page-title">
    <h1 class="title">Epoch Programming Language</h1>
    <h3 class="subtitle">Building an Opinionated Language for Software Development</h3>
  </div>

  <div class="navbar">` +
    '<a class="navlink" href=' + relativeHomeLink + '>Home</a>' +
    '<a class="navlink" href=' + relativeOverviewLink + '>Overview</a>' +
    '<a class="navlink" href=' + relativeArticlesLink + '>Articles</a>' +
    '<a class="navlink" href="https://github.com/apoch/epoch-language/releases">Releases</a>' +
    `<a class="navlink" href="https://github.com/apoch/epoch-language">Source</a>
  </div>`;
};

function getLevelsDeep(stoppingPoint) {
  // pathname will return the full path on a host machine
  // but only to the hostname on a website
  // we want href for now, probably, since the host site
  // also has an 'epoch-language' that we're stopping at
  // but we'll need to leave a note for changing things
  // for an actual domain
  let urlArray = window.location.pathname.split('/'); //www.web.com + folder + file.html
  
  //change this to 'window.location.hostname'
  //when epoch has its own website
  //let stoppingPoint = "epoch-language"; 
  
  //alert(urlArray + " and length is " + urlArray.length);
  
  let levelsDeep = 0;
  let i = urlArray.length - 2; //to skip the page we are - e.g. index.html
  
  while(i >= 0 && urlArray[i] !== stoppingPoint){
    levelsDeep += 1;
    --i;
  }
  
  //alert("Levels deep: " + levelsDeep);
  
  return levelsDeep;
}

window.onload = function() {
    var myHeader = document.registerElement('my-header', {prototype: myHeaderPrototype});
    
};

