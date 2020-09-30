// Enable latex render
const math = require('remark-math')
const katex = require('rehype-katex')

module.exports = {
  title: 'Dune Copasi',
  tagline: 'Solver for reaction-diffusion systems in multiple compartments',
  url: 'https://dune-copasi.netlify.app/',
  baseUrl: '/',
  onBrokenLinks: 'throw',
  favicon: 'img/favicon.ico',
  organizationName: 'dune-copasi',
  projectName: 'dune-copasi',

  // CSS for latex
  stylesheets: [
    {
      href: 'https://cdn.jsdelivr.net/npm/katex@0.11.1/dist/katex.min.css',
      type: 'text/css',
      integrity: 'sha384-zB1R0rpPzHqg7Kpt0Aljp8JPLqbXI3bhnPWROx27a9N0Ll6ZP/+DiW/UqRcLbRjq',
      crossorigin: 'anonymous',
    },
  ],

  themeConfig: {
    // Syntax highlight
    prism: {
      additionalLanguages: ['ini'],
    },
    announcementBar: {
      id: 'supportus',
      content:
        '⭐️ If you like DuneCopasi, give it a star on <a target="_blank" rel="noopener noreferrer" href="https://gitlab.dune-project.org/copasi/dune-copasi">GitLab</a> or <a target="_blank" rel="noopener noreferrer" href="https://github.com/dune-copasi/dune-copasi">GitHub</a>! ⭐️',
    },
    navbar: {
      title: 'DuneCopasi',
      logo: {
        alt: 'dune-copasi',
        src: 'img/logo.svg',
      },
      items: [
        {
          to: 'docs/',
          activeBasePath: 'docs',
          label: 'Learn',
          position: 'left',
        },
        // {to: 'blog', label: 'Blog', position: 'left'},
        {
          href: 'https://gitlab.dune-project.org/copasi/dune-copasi',
          label: 'GitLab',
          // className: 'header-gitlab-link', // TODO
          position: 'right',
          'aria-label': 'GitLab repository',
        },
        {
          href: 'https://github.com/dune-copasi/dune-copasi',
          position: 'right',
          // label: 'GitHub',
          className: 'header-github-link',
          'aria-label': 'GitHub repository',
        },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: 'Learn',
          items: [
            {
              label: 'Introduction',
              to: 'docs/',
            },
            {
              label: 'Get Started',
              to: 'docs/install_use',
            },
          ],
        },
      ],
    },
  },
  presets: [
    [
      '@docusaurus/preset-classic',
      {
        docs: {
          homePageId: 'intro',
          sidebarPath: require.resolve('./sidebars.js'),
          editUrl:
          'https://gitlab.dune-project.org/copasi/dune-copasi/-/edit/master/doc/docusaurus/',
          remarkPlugins: [math],
          rehypePlugins: [katex],
        },
        blog: {
          showReadingTime: true,
          editUrl:
          'https://gitlab.dune-project.org/copasi/dune-copasi/-/edit/master/doc/docusaurus/',
        },
        theme: {
          customCss: require.resolve('./src/css/custom.css'),
        },
      },
    ],
  ],
};
