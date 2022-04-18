from setuptools import setup

setup(
    name='atom32_provisioning',
    version='1.0.0',
    url='',
    license='',
    author='',
    install_requires=[
        'pyserial>=3.4',
        'esptool>=2.6',
        'requests>=2.0',
        'gpiozero>=1.0'
        ],
    author_email='',
    description='Automated provisioning script for Atom32 boards',
    long_description='',
    entry_points={
        'console_scripts': [
            'atom32_provisioning = atom32_provisioning.scripts.atom32_provisioning_cli:main',
            ],
        },
    packages=['atom32_provisioning', 'atom32_provisioning.scripts'],
    include_package_data=True,
    platforms='any',
    zip_safe=False,
    # package_data={'atom32_provisioning': ['']},
    classifiers=[
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'Development Status :: 1 - Alpha',
        'Natural Language :: English',
        'Intended Audience :: Developers',
        'Operating System :: OS Independent',
        ],
)

