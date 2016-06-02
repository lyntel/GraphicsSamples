# Javascripts for NVIDIA Dev Zone front end

# A simple routine to queue a callback in case there are timing issues (e.g. getting the size of the window returns the wrong one)
queueCallback = (callback) ->
  setTimeout(callback, 100)


# Dom ready
jQuery ->
  # Set all background images
  jQuery('[data-background-image]').each ->
    bg = jQuery(this).data('background-image')
    jQuery(this).css('background-image', "url(#{bg})")

  # Google Prettyprint
  prettyPrint()

  # Fitvids responsive videos
  jQuery('[data-responsive-video=true]').fitVids()

  # Colorbox
  jQuery('a[data-colorbox=true]').colorbox({
    maxWidth: '90%',
    maxHeight: '90%'
  })
  jQuery('a[data-colorbox-slideshow]').each ->
    gallery = jQuery(this).data('colorbox-slideshow')
    jQuery(this).colorbox({ 
      rel: gallery,
      maxWidth: '90%',
      maxHeight: '90%'
    })


  # Flynav/special navigation
  menuToggled = null

  mobileMenuWidth = ->
    if jQuery(window).width() > 479
      return '40%'
    else
      return '80%'

  # Flynav
  jQuery('.navbar-toggle').sidr({
    source: '#navbar-collapse',
    renaming: false
    })

  # Search bar events
  # Returns the available width in the navbar
  availableNavbarWidth = () ->
    jQuery('#navbar-collapse').width() - jQuery('#navbar-items').width() - jQuery('.navbar-header').width() - jQuery('.login-nav').width()
  
  searchBarVisible = false
  
  jQuery('#trigger-search-top').on "click", ->
    searchBarVisible = !searchBarVisible
    searchBarWidth = 300
    
    jQuery("#search-top-bar").toggle (->
      jQuery(this).animate
        'width': 0
        'height': '36px'
        'opacity': 0
      , 150
    ), ->
      jQuery(this).animate
        'width': searchBarWidth
        'height': '36px'
        'opacity': 1
      , 150

    # Fade in/out items on navbar
    # Figure out if we have to fade the items out or not
    navbar = jQuery('#navbar-items')
    if searchBarVisible
      if availableNavbarWidth() < searchBarWidth
        navbar.fadeOut()
      # Focus on search
      jQuery('#search-top-bar').focus()
    else
      if !navbar.is(':visible')
        navbar.fadeIn()

  jQuery("#search-top-bar").focusout ->
    # If no text is contained within the search bar, and the user clicks away then it hides the search bar
    if jQuery("#search-top-bar").val() == ''

      searchBarVisible = !searchBarVisible
      searchBarWidth = 300

      jQuery("#search-top-bar").toggle (->
        jQuery(this).animate
          'width': 0
          'height': '36px'
          'opacity': 0
        , 150
      ), ->
        jQuery(this).animate
          'width': searchBarWidth
          'height': '36px'
          'opacity': 1
        , 150

      # Fade in/out items on navbar
      # Figure out if we have to fade the items out or not
      navbar = jQuery('#navbar-items')
      if searchBarVisible
        if availableNavbarWidth() < searchBarWidth
          navbar.fadeOut()
        # Focus on search
        jQuery('#search-top-bar').focus()
      else
        if !navbar.is(':visible')
          navbar.fadeIn()

  # Callback method for Positioning Owl Controls
  positionOwlControls = () ->
    mastheadHeight = jQuery('#owl-carousel').height()
    controlHeight = jQuery('.owl-prev').height()
    offset = (mastheadHeight - controlHeight)/2
    jQuery('.owl-prev').css('top', "#{offset}px")
    jQuery('.owl-next').css('top', "#{offset}px")

  # Callback method for maximizing height of Owl items
  maximizeOwlItemHeights = () ->
    mastheadHeight = jQuery('#owl-carousel').height()
    jQuery('#owl-carousel .item').css('min-height', mastheadHeight);

  # Set up Owl Carousel
  jQuery("#owl-carousel").owlCarousel
    rewindNav : false
    navigation: true
    navigationText: ["<i class=\"fa fa-angle-left hidden-xs\"></i>", "<i class=\"fa fa-angle-right hidden-xs\"></i>"]
    pagination : false
    slideSpeed: 300
    paginationSpeed: 400
    singleItem: true,
    afterUpdate: ->
      queueCallback(positionOwlControls)
      queueCallback(maximizeOwlItemHeights)
    afterInit: ->
      queueCallback(positionOwlControls)
      queueCallback(maximizeOwlItemHeights)

  # Set up Isotope
  # Only run this if we have isotope on a page
  if jQuery('[data-isotope-container=true]').length > 0
    jQuery('[data-isotope-container=true]').isotope({
      layoutMode: 'fitRows',
      animationEngine: 'jquery'
    })
    # Hack for Safari to trigger the resize so that everything renders
    jQuery(window).trigger('resize')

  # Crop 4:3 YouTube thumbnails into 16:9
  # Note that there are a lot of shenannigans here to make this work cross browser
  cropThumbnails = ->
    aspect_16_9 = 1.77777777777778
    aspect_4_3 = 1.33333333333333
    jQuery('[data-crop-video-thumbnail=true]').each ->
      # Assume that the image from YouTube is 4:3
      width = jQuery(this).width()
      targetHeight = width / aspect_16_9
      actualHeight = width / aspect_4_3
      marginTop = -(actualHeight - targetHeight) / 2
      jQuery(this).css('overflow', 'hidden').css('height', targetHeight)
      jQuery(this).find('img').css('margin-top', marginTop)
  cropThumbnails()
  jQuery(window).resize(cropThumbnails)

# When user resizes browser
jQuery(window).resize ->
  if jQuery(window).width() < 1024
    jQuery("#navbar-items").attr 'style', ''

  if jQuery(window).width() > 767
    jQuery('#navbar-collapse, body, #wrapper, .navbar-header').attr 'style', ''
    jQuery("#search-top-bar").attr 'style', ''
  else
    jQuery('body').css "width", jQuery(window).width()