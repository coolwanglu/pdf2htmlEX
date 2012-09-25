/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */
/* vim modeline copied from pdf.js */

/*
 * pdf2htmlEX.js
 *
 * handles UI events/actions/effects
 *
 * Copyright 2012 Lu Wang <coolwanglu@gmail.com>
 */

var pdf2htmlEX = (function(){
  var EPS = 1e-6;
  var invert = function(ctm) {
    var det = ctm[0] * ctm[3] - ctm[1] * ctm[2];
    var ictm = new Array();
    ictm[0] = ctm[3] / det;
    ictm[1] = -ctm[1] / det;
    ictm[2] = -ctm[2] / det;
    ictm[3] = ctm[0] / det;
    ictm[4] = (ctm[2] * ctm[5] - ctm[3] * ctm[4]) / det;
    ictm[5] = (ctm[1] * ctm[4] - ctm[0] * ctm[5]) / det;
    return ictm;
  };
  var transform = function(ctm, pos) {
    return [ctm[0] * pos[0] + ctm[2] * pos[1] + ctm[4]
           ,ctm[1] * pos[0] + ctm[3] * pos[1] + ctm[5]];
  };
  var Page = function(page) {
    if(page == undefined) return undefined;

    this.p = $(page);
    this.n = parseInt(this.p.attr('data-page-no'), 16);
    this.b = $('.b', this.p);

    /*
     * scale ratios
     * 
     * default_r : the first one
     * set_r : last set 
     * cur_r : currently using
     */
    this.default_r = this.set_r = this.cur_r = this.p.height() / this.b.height();

    this.data = JSON.parse($($('.j', this.p)[0]).attr('data-data'));

    this.ctm = this.data.ctm;
    this.ictm = invert(this.ctm);
  };
  Page.prototype.hide = function(){
    this.b.hide();
  };
  Page.prototype.show = function(){
    if(Math.abs(this.set_r - this.cur_r) > EPS) {
      this.cur_r = this.set_r;
      this.b.css('transform', 'scale('+this.cur_r.toFixed(3)+')');
    }
    this.b.show();
  };
  Page.prototype.rescale = function(ratio, is_relative) {
    if(ratio == 0) {
      this.set_r = this.default_r;
    } else if (is_relative) {
      this.set_r *= ratio;
    } else {
      this.set_r = ratio;
    }

    /* wait for redraw */
    this.hide();

    this.p.height(this.b.height() * this.set_r);
    this.p.width(this.b.width() * this.set_r);
  };
  Page.prototype.is_visible = function() {
    var p = this.p;
    var off = p.position();
    return !((off.top + p.height() < 0) || (off.top > p.offsetParent().height()));
  };

  return {
    pages : [],
    container : null,
    render_timer : null,

    /* Constants */
    render_timeout : 200,
    scale_step : 0.9,

    init_before_loading_content : function() {
      /*hide all pages before loading, will reveal only visible ones later */
      this.pre_hide_pages();
    },

    init_after_loading_content : function() {
      this.container = $('#pdf-main');

      var new_pages = new Array();
      var pl= $('.p', this.container);
      /* don't use for(..in..) */
      for(var i = 0, l = pl.length; i < l; ++i) {
        var p = new Page(pl[i]);
        new_pages[p.n] = p;
      }
      this.pages = new_pages;

      var _ = this;
      this.container.scroll(function(){ _.schedule_render(); });

      //this.zoom_fixer();
      
      this.container.on('click', '.a', this, this.annot_link_handler);

      this.render();
    },

    init : function() {
      this.init_before_loading_content();

      var _ = this;
      $(function(){_.init_after_loading_content();});

      return this;
    },

    pre_hide_pages : function() {
      /* pages might have not been loaded yet, so add a CSS rule */
      var s = '.b{display:none;}';
      var n = document.createElement('style');
      n.type = 'text/css';
      if (n.styleSheet) {
        n.styleSheet.cssText = s;
      } else {
        n.appendChild(document.createTextNode(s));
      }
      document.getElementsByTagName('head')[0].appendChild(n);
    },

    hide_pages : function() {
      var pl = this.pages;
      for(var i in pl)
        pl[i].hide();
    },

    render : function () {
      /* hide (positional) invisible pages */
      var pl = this.pages;
      for(var i in pl) {
        var p = pl[i];
        if(p.is_visible())
          p.show();
        else
          p.hide();
      }
    },

    schedule_render : function() {
      if(this.render_timer)
        clearTimeout(this.render_timer);

      var _ = this;
      this.render_timer = setTimeout(function () {
        _.render();
      }, this.render_timeout);
    },

    zoom_fixer : function () {
      /* 
       * When user try to zoom in/out using ctrl + +/- or mouse wheel
       * handle this and prevent the default behaviours
       *
       * Code credit to PDF.js
       */
      var _ = this;
      // Firefox specific event, so that we can prevent browser from zooming
      $(window).on('DOMMouseScroll', function(e) {
        if (e.ctrlKey) {
          e.preventDefault();
          _.rescale(Math.pow(_.scale_step, e.detail), true);
        }
      });

      $(window).on('keydown', function keydown(e) {
        if (e.ctrlKey || e.metaKey) {
          switch (e.keyCode) {
            case 61: // FF/Mac '='
            case 107: // FF '+' and '='
            case 187: // Chrome '+'
              _.rescale(1.0 / _.scale_step, true);
              break;
            case 173: // FF/Mac '-'
            case 109: // FF '-'
            case 189: // Chrome '-'
              _.rescale(_.scale_step, true);
              break;
            case 48: // '0'
              _.rescale(0, false);
              break;
            default:
              return;
          }
          e.preventDefault();
        }
      });
    },

    rescale : function (ratio, is_relative) {
      var pl = this.pages;
      for(var i in pl) {
        pl[i].rescale(ratio, is_relative);
      }

      this.schedule_render();
    },

    get_containing_page : function(obj) {
      /* get the page obj containing obj */
      return this.pages[(new Page(obj.closest('.p')[0])).n];
    },

    annot_link_handler : function (e) {
      var _ = e.data;
      var t = $(e.currentTarget);
      var cur_page = _.get_containing_page(t);
      if(cur_page == undefined) return;
    
      var off = cur_page.p.position();
      var cur_pos = transform(cur_page.ictm, [-off.left, cur_page.p.height()+off.top]);

      var detail_str = t.attr('data-dest-detail');
      if(detail_str == undefined) return;

      var ok = false;
      var detail= JSON.parse(detail_str);
      var target_page = _.pages[detail[0]];
      if(target_page == undefined) return;

      switch(detail[1]) {
        case 'XYZ':
          var pos = [(detail[2] == null) ? cur_pos[0] : detail[2]
                    ,(detail[3] == null) ? cur_pos[1] : detail[3]];
          pos = transform(cur_page.ctm, pos);

          console.log(pos);

          var off = target_page.p.position();

          console.log(off);

          _.container.scrollLeft(_.container.scrollLeft()+off.left+pos[0]);
          _.container.scrollTop(_.container.scrollTop()+off.top+target_page.p.height()-pos[1]);
          ok = true;
          break;
        default:
          break;
      }

      if(ok)
        e.preventDefault();
    }, __last_member__ : 'no comma' /*,*/
  }.init();
})();
